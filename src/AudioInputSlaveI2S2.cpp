#include "AudioInputSlaveI2S2.h"
#include <Arduino.h>

// Static Definitions
DMAChannel AudioInputSlaveI2S2::dma;
uint32_t
    AudioInputSlaveI2S2::i2s_rx_buffer[512]; // 512 words (256 Stereo Frames)
volatile uint32_t AudioInputSlaveI2S2::isrCount = 0;
audio_block_t *AudioInputSlaveI2S2::block_left = NULL;
audio_block_t *AudioInputSlaveI2S2::block_right = NULL;
uint16_t AudioInputSlaveI2S2::block_offset = 0;
audio_block_t *AudioInputSlaveI2S2::inputQueueArray[2];

void AudioInputSlaveI2S2::begin(void) {
  dma.begin(true); // Allocate DMA Channel

  // 1. Clock Setup
  CCM_CCGR5 |= CCM_CCGR5_SAI2(CCM_CCGR_ON);

  // 2. Mux Setup
  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_08 = 2; // Data
  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_08 = 0x10B0;

  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_06 = 2 | 0x10; // BCLK + SION
  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_06 = 0x10B0;

  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_05 = 2 | 0x10; // Sync + SION
  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_05 = 0x10B0;

  IOMUXC_SAI2_RX_DATA0_SELECT_INPUT = 0;
  // Input Selects not strictly needed for Master but good for safety
  IOMUXC_SAI2_TX_BCLK_SELECT_INPUT = 0;
  IOMUXC_SAI2_TX_SYNC_SELECT_INPUT = 0;

  // 3. Register Setup
  IMXRT_SAI2.TCSR = 0;
  IMXRT_SAI2.RCSR = 0;

  // TX Config (Clock Provider)
  // BCD=0 (Slave) - unused if RX is Async, but good practice to allow Input on
  // pins
  IMXRT_SAI2.TCR2 = I2S_TCR2_MSEL(1) | I2S_TCR2_BCP; // BCD=0
  IMXRT_SAI2.TCR3 = I2S_TCR3_TCE;
  IMXRT_SAI2.TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(31) | I2S_TCR4_MF |
                    I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD;
  IMXRT_SAI2.TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

  // RX Config (Asynchronous Slave)
  IMXRT_SAI2.RMR = 0;
  IMXRT_SAI1.RCR1 = I2S_RCR1_RFW(1);
  // RCR2: SYNC=0 (Async), BCD=0 (Slave/External), MSEL=1 (Bus Clock - unused?
  // or interal logic), BCP=1 (Active Low)
  IMXRT_SAI2.RCR2 = I2S_RCR2_SYNC(0) | I2S_RCR2_MSEL(1) |
                    I2S_RCR2_BCP; // Remove BCD (Slave) and SYNC (Async)
  IMXRT_SAI2.RCR3 = I2S_RCR3_RCE;
  IMXRT_SAI2.RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(31) | I2S_RCR4_MF |
                    I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
  IMXRT_SAI2.RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

  // 4. DMA Configuration (EXPLICIT TCD)
  // This fixes the Static Noise by ensuring SOFF=0 (Fixed Source Address)
  dma.TCD->SADDR = (void *)((uint32_t)&IMXRT_SAI2.RDR[0]);
  dma.TCD->SOFF = 0;
  dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
  dma.TCD->NBYTES_MLNO = 4;
  dma.TCD->SLAST = 0;

  dma.TCD->DADDR = i2s_rx_buffer;
  dma.TCD->DOFF = 4;
  dma.TCD->CITER_ELINKNO = sizeof(i2s_rx_buffer) / 4;
  dma.TCD->DLASTSGA = -sizeof(i2s_rx_buffer);
  dma.TCD->BITER_ELINKNO = sizeof(i2s_rx_buffer) / 4;

  dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

  dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_RX);
  dma.enable();

  // 5. Enable
  IMXRT_SAI2.RCSR |= I2S_RCSR_RE | I2S_RCSR_FRDE;
  IMXRT_SAI2.TCSR =
      I2S_TCSR_TE | I2S_TCSR_BCE; // Enable TX + Bit Clock Generation

  dma.attachInterrupt(isr);
}

void AudioInputSlaveI2S2::update(void) {
  audio_block_t *l = NULL, *r = NULL;

  __disable_irq();
  if (block_left && block_right) {
    l = block_left;
    r = block_right;
    block_left = NULL;
    block_right = NULL;
  }
  __enable_irq();

  if (l && r) {
    transmit(l, 0);
    transmit(r, 1);
    release(l);
    release(r);
  }
}

void AudioInputSlaveI2S2::isr(void) {
  isrCount++;
  dma.clearInterrupt();

  uint32_t *src;

  if ((uint32_t)dma.TCD->DADDR < (uint32_t)&i2s_rx_buffer[256]) {
    src = &i2s_rx_buffer[256];
  } else {
    src = &i2s_rx_buffer[0];
  }

  // Invalidate Cache (Fixes Static/Corruption)
  arm_dcache_delete(src, 256 * sizeof(uint32_t));

  audio_block_t *l = AudioStream::allocate();
  audio_block_t *r = AudioStream::allocate();

  if (!l || !r) {
    if (l)
      AudioStream::release(l);
    if (r)
      AudioStream::release(r);
    return;
  }

  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    uint32_t val1 = src[i * 2];
    uint32_t val2 = src[i * 2 + 1];
    l->data[i] = (int16_t)(val1 >> 16);
    r->data[i] = (int16_t)(val2 >> 16);
  }

  AudioInputSlaveI2S2::block_left = l;
  AudioInputSlaveI2S2::block_right = r;
  AudioInputSlaveI2S2::block_offset = AUDIO_BLOCK_SAMPLES;

  AudioStream::update_all();
}
