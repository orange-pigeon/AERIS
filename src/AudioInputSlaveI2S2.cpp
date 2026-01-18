#include "AudioInputSlaveI2S2.h"
#include "utility/imxrt_hw.h"

// Static member definitions
volatile uint32_t AudioInputSlaveI2S2::isrCount = 0;
DMAChannel AudioInputSlaveI2S2::dma(false);
bool AudioInputSlaveI2S2::update_responsibility = false;
audio_block_t *AudioInputSlaveI2S2::block_left = NULL;
audio_block_t *AudioInputSlaveI2S2::block_right = NULL;
uint16_t AudioInputSlaveI2S2::block_offset = 0;

// DMA Buffer for 2 slots (L, R) at 44.1kHz
// The QCC5125 is sending 48 bits per frame (24 bits per slot).
#define I2S_SLOTS 2
DMAMEM __attribute__((aligned(
    32))) static uint32_t i2s2_rx_buffer[AUDIO_BLOCK_SAMPLES * I2S_SLOTS];

// Global instance used by the system
AudioInputSlaveI2S2 qccAudio;

void AudioInputSlaveI2S2::begin(void) {
  dma.begin(true); // Allocate DMA channel

  config_i2s();

  // DMA source: SAI2 Receive Data Register 0
  dma.TCD->SADDR = (void *)&I2S2_RDR0;
  dma.TCD->SOFF = 0;
  dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2); // 32-bit
  dma.TCD->NBYTES_MLNO = 4;
  dma.TCD->SLAST = 0;
  dma.TCD->DADDR = i2s2_rx_buffer;
  dma.TCD->DOFF = 4;

  // 256 transfers = 128 stereo samples
  uint32_t totalTransfers = AUDIO_BLOCK_SAMPLES * I2S_SLOTS;
  dma.TCD->CITER_ELINKNO = totalTransfers;
  dma.TCD->BITER_ELINKNO = totalTransfers;
  dma.TCD->DLASTSGA = -(totalTransfers * 4);

  dma.TCD->CSR = DMA_TCD_CSR_INTMAJOR;

  dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_RX);
  dma.enable();

  I2S2_RCSR = I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
  I2S2_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE;

  dma.attachInterrupt(isr);
}

void AudioInputSlaveI2S2::config_i2s(void) {
  // Enable SAI2 clock
  CCM_CCGR5 |= CCM_CCGR5_SAI2(CCM_CCGR_ON);

  // Skip configuration if already running
  if (I2S2_TCSR & I2S_TCSR_TE)
    return;
  if (I2S2_RCSR & I2S_RCSR_RE)
    return;

  // Pin Muxing for SAI2 (I2S2)
  CORE_PIN4_CONFIG = 2; // BCLK2
  CORE_PIN3_CONFIG = 2; // LRCLK2
  CORE_PIN5_CONFIG = 2; // IN2

  // Daisy chain selection for inputs
  IOMUXC_SAI2_RX_BCLK_SELECT_INPUT = 0;
  IOMUXC_SAI2_RX_SYNC_SELECT_INPUT = 0;
  IOMUXC_SAI2_RX_DATA0_SELECT_INPUT = 0;

  // Slave Configuration: BCD=0 (Slave), FSD=0 (Slave)
  // Synchronize RX with TX clock pins (rsync=1)
  int rsync = 1;
  int tsync = 0;

  // IMPORTANT: Set to 24 bits because 48 BCLK/frame = 2x24 bits
  uint32_t wordSize = 24;
  uint32_t wordSizeM1 = wordSize - 1;

  // Transmitter config
  I2S2_TMR = 0;
  I2S2_TCR1 = I2S_TCR1_RFW(1);
  I2S2_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP;
  I2S2_TCR3 = I2S_TCR3_TCE;
  // 2 slots per frame (FRSZ=1)
  I2S2_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(wordSizeM1) | I2S_TCR4_MF |
              I2S_TCR4_FSE | I2S_TCR4_FSP;
  I2S2_TCR5 =
      I2S_TCR5_WNW(wordSizeM1) | I2S_TCR5_W0W(wordSizeM1) | I2S_TCR5_FBT(31);

  // Receiver config
  I2S2_RMR = 0; // No hard masking
  I2S2_RCR1 = I2S_RCR1_RFW(1);
  I2S2_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_TCR2_BCP;
  I2S2_RCR3 = I2S_RCR3_RCE;
  I2S2_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(wordSizeM1) | I2S_RCR4_MF |
              I2S_RCR4_FSE | I2S_RCR4_FSP;
  I2S2_RCR5 =
      I2S_RCR5_WNW(wordSizeM1) | I2S_RCR5_W0W(wordSizeM1) | I2S_RCR5_FBT(31);
}

void AudioInputSlaveI2S2::isr(void) {
  audio_block_t *left, *right;

  dma.clearInterrupt();
  isrCount++;

  left = block_left;
  right = block_right;

  if (left != NULL && right != NULL) {
    uint32_t *src = i2s2_rx_buffer;
    int16_t *dest_left = left->data;
    int16_t *dest_right = right->data;

    // Invalidate cache for the source buffer before reading
    arm_dcache_delete((void *)src, sizeof(i2s2_rx_buffer));

    // For 24-bit slots, the data is usually in the MSB of the 32-bit pop.
    // Shift right by 16 bits to get a 16-bit signed sample.
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      *dest_left++ = (int16_t)((*src++) >> 16);
      *dest_right++ = (int16_t)((*src++) >> 16);
    }

    block_offset = AUDIO_BLOCK_SAMPLES;
  }
}

void AudioInputSlaveI2S2::update(void) {
  audio_block_t *new_left = NULL, *new_right = NULL, *out_left = NULL,
                *out_right = NULL;

  // Allocate 2 new blocks
  new_left = allocate();
  if (new_left != NULL) {
    new_right = allocate();
    if (new_right == NULL) {
      release(new_left);
      new_left = NULL;
    }
  }

  __disable_irq();
  if (block_offset >= AUDIO_BLOCK_SAMPLES) {
    // We have a full block of samples ready
    out_left = block_left;
    out_right = block_right;

    block_left = new_left;
    block_right = new_right;
    block_offset = 0;
    __enable_irq();

    if (out_left) {
      transmit(out_left, 0);
      release(out_left);
    }
    if (out_right) {
      transmit(out_right, 1);
      release(out_right);
    }
  } else if (new_left != NULL) {
    // We allocated new blocks but don't have a full block yet
    if (block_left == NULL) {
      // Fill current vacancy
      block_left = new_left;
      block_right = new_right;
      block_offset = 0;
      __enable_irq();
    } else {
      // Already have blocks waiting for more data
      __enable_irq();
      release(new_left);
      release(new_right);
    }
  } else {
    __enable_irq();
  }
}
