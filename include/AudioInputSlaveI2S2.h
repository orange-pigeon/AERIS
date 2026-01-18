#ifndef AUDIO_INPUT_SLAVE_I2S2_H
#define AUDIO_INPUT_SLAVE_I2S2_H

#include <Arduino.h>
#include <Audio.h>
#include <AudioStream.h>
#include <DMAChannel.h>

class AudioInputSlaveI2S2 : public AudioStream {
public:
  AudioInputSlaveI2S2() : AudioStream(0, NULL) { begin(); }
  virtual void update(void);
  void begin(void);
  static volatile uint32_t isrCount; // Debug counter

private:
  static DMAChannel dma;
  static void config_sai2();
  static void config_dma();
  static void isr(void);

  // DMA Buffer: 2 channels (L/R) * 128 samples/block * 2 (Ping/Pong)
  // 32-bit width used for transfer, though audio is 16-bit packed or 32-bit
  static uint32_t i2s_rx_buffer[256 * 2];

  static audio_block_t *block_left;
  static audio_block_t *block_right;
  static uint16_t block_offset;
  static audio_block_t *inputQueueArray[2];
};

#endif
