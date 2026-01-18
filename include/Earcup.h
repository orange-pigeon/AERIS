#pragma once
#include <Audio.h>

class Earcup : public AudioStream {
public:
  Earcup();

  // 1) Mic gains (alleen microfoon gevoeligheid/kalibratie)
  void setOutsideMicGain(float gain);
  void setErrorMicGain(float gain);

  float getOutsideMicGain() const;
  float getErrorMicGain() const;

  // 2) Mix levels (hoeveel gaat er in de output t.o.v. inAudio)
  void setSidetoneLevel(float level); // 0..1
  float getSidetoneLevel() const;

  // 3) Enable switches
  void setSidetoneEnabled(bool enabled);
  bool isSidetoneEnabled() const;

protected:
  void update() override;

private:
  // [0] outside mic
  // [1] error mic
  // [2] mono program audio in (van Mixer) and the final output mixer (Mixer).
  audio_block_t *inputQueueArray[3];

  // Q15 (0..32767)
  volatile int32_t outsideMicGainQ15_ = 32767; // 1.0
  volatile int32_t errorMicGainQ15_ = 32767;   // 1.0

  volatile int32_t sidetoneLevelQ15_ = 0; // 0.0
  volatile int32_t errorLevelQ15_ = 0;    // 0.0

  volatile bool sidetoneEnabled_ = false;

  void process(int16_t *outAudio, const int16_t *outside, const int16_t *error,
               const int16_t *inAudio, size_t sampleCount);

  static inline int16_t clamp16(int32_t x) {
    if (x > 32767)
      return 32767;
    if (x < -32768)
      return -32768;
    return (int16_t)x;
  }

  static inline int32_t floatToQ15(float g) {
    if (g < 0.0f)
      g = 0.0f;
    if (g > 1.0f)
      g = 1.0f;
    return (int32_t)(g * 32767.0f + 0.5f);
  }

  static inline float q15ToFloat(int32_t q) {
    if (q < 0)
      q = 0;
    if (q > 32767)
      q = 32767;
    return (float)q / 32767.0f;
  }
};