#ifndef AudioInputSlaveI2S2_h_
#define AudioInputSlaveI2S2_h_

#include <Arduino.h>
#include <AudioStream.h>
#include <DMAChannel.h>

/**
 * AudioInputSlaveI2S2
 *
 * Custom I2S Input class for Teensy 4.1 using SAI2 (I2S2) in Slave Mode.
 * In Slave Mode, the external device (e.g., Bluetooth module) must provide
 * BCLK and LRCLK signals.
 *
 * Pins used on Teensy 4.1:
 * - Pin 3: LRCLK (SAI2_TX_SYNC)
 * - Pin 4: BCLK (SAI2_TX_BCLK)
 * - Pin 5: IN (SAI2_RX_DATA0)
 */
class AudioInputSlaveI2S2 : public AudioStream {
public:
  AudioInputSlaveI2S2(void) : AudioStream(0, NULL) { begin(); }
  virtual void update(void);
  void begin(void);

  // Debugging counter to verify ISR execution and clock stability
  static volatile uint32_t isrCount;

private:
  static void isr(void);
  static DMAChannel dma;
  static bool update_responsibility;
  static audio_block_t *block_left;
  static audio_block_t *block_right;
  static uint16_t block_offset;

  void config_i2s(void);
};

// Extern declaration for convenient usage (used in main.cpp)
extern AudioInputSlaveI2S2 qccAudio;

#endif
