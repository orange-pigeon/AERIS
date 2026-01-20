#pragma once

#include "Mixer.h"
#include <Arduino.h>
#include <Audio.h>
#include <initializer_list>

/**
 * @class SinePlayer
 * @brief Manages a pool of sine wave generators with support for timed
 * playback.
 *
 * This class allows playing multiple independent tones simultaneously for a
 * specified duration. It automatically handles stopping the tones once the
 * time has elapsed.
 */
class SinePlayer {
public:
  static const int POOL_SIZE = 4; ///< Number of concurrent sine generators.

  SinePlayer(Mixer &outputMixer);

  /**
   * @brief Initializes the sine generators and connects them to the output
   * mixer.
   */
  void init();

  /**
   * @brief Configures which mixer slots this manager should use.
   * Should be called before init().
   * @param slots An initializer list of slot indices.
   */
  void setMixerSlots(std::initializer_list<int8_t> slots);

  /**
   * @brief Plays a sine wave on the first available generator slot.
   * @param freq Frequency of the tone in Hertz.
   * @param durationMs Duration in milliseconds. If 0, the tone plays
   * indefinitely.
   */
  void play(float freq, uint32_t durationMs = 0);

  /**
   * @brief Stops all active sine waves immediately.
   */
  void stopAll();

  /**
   * @brief Manages tone durations. Must be called repeatedly in the main loop.
   */
  void update();

private:
  Mixer &outputMixer_;                  ///< Reference to the main audio mixer.
  AudioSynthWaveform sines_[POOL_SIZE]; ///< Pool of waveform generator objects.
  uint32_t stopTimes_[POOL_SIZE]; ///< The millis() timestamp when each tone
                                  ///< should stop.
  bool active_[POOL_SIZE];        ///< Tracks which slots are currently "busy".
  int8_t
      mixerSlots_[POOL_SIZE]; ///< Maps manager slot index to mixer input index.
  int8_t requestedSlots_[POOL_SIZE]; ///< User-requested mixer slots (-1 =
                                     ///< dynamic).

  /**
   * @brief Finds the first generator slot that is not currently active.
   */
  int findFreeSlot();
};
