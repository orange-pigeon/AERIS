#pragma once

#include <Arduino.h>
#include <Audio.h>

/**
 * @class Mixer
 * @brief A custom 8-channel audio mixer with built-in side-chain ducking and
 * source naming.
 *
 * This mixer extends the Teensy AudioStream to allow dynamic connection
 * management, individual channel gains, and automatic volume reduction
 * (ducking) based on another channel's signal level.
 */
class Mixer : public AudioStream {
public:
  static constexpr uint8_t MAX_INPUTS = 10;

  Mixer();

  /**
   * @brief Connects an audio source to the next available mixer slot.
   * @param source The AudioStream source to connect.
   * @param sourceChannel The channel index on the source (usually 0).
   * @return The slot index (0-7) if successful, or -1 if the mixer is full.
   */
  int8_t addInput(AudioStream &source, uint8_t sourceChannel = 0);

  /**
   * @brief Disconnects the source at the specified slot and frees the index.
   * @param slot The mixer slot index to remove.
   */
  void removeInput(uint8_t slot);

  /**
   * @brief Sets the gain for a specific input slot.
   * @param slot Mixer slot index (0-7).
   * @param gain Volume multiplier (0.0 to 1.0).
   */
  void setGain(uint8_t slot, float gain);

  /**
   * @brief Returns the current gain of a specific input slot.
   * @param slot Mixer slot index (0-7).
   * @return Gain value (0.0 to 1.0).
   */
  float getGain(uint8_t slot) const;

  /**
   * @struct DuckingConfig
   * @brief Settings for the side-chain ducking effect on a specific channel.
   */
  struct DuckingConfig {
    int8_t controlSlot =
        -1; ///< Slot index that triggers the ducking (-1 = disabled).
    float duckingGain =
        0.2f; ///< Target volume ratio during active ducking (e.g., 0.2 = 20%).
    float threshold =
        0.05f; ///< Signal level (0.0 - 1.0) required to trigger ducking.
    float attack = 0.1f;    ///< Fade-down speed coëfficient.
    float release = 0.005f; ///< Fade-up speed coëfficient.
    float currentScale =
        1.0f; ///< Current real-time gain multiplier (1.0 = no reduction).
  };

  /**
   * @brief Configures ducking for a target channel.
   * @param targetSlot The channel that should be attenuated (e.g., background
   * music).
   * @param controlSlot The channel that triggers the attenuation (e.g.,
   * voice/sine).
   * @param duckGain Volume multiplier applied during ducking (0.0 to 1.0).
   * @param threshold Peak level on control channel needed to trigger (0.0
   * to 1.0).
   * @param attackMs Time to reach ducked volume in milliseconds.
   * @param releaseMs Time to return to normal volume in milliseconds.
   */
  void setDucking(uint8_t targetSlot, int8_t controlSlot, float duckGain,
                  float threshold, float attackMs, float releaseMs);

  /**
   * @brief Disconnects all current inputs and resets naming.
   */
  void clear();

  /**
   * @brief Assigns a human-readable name to a mixer slot.
   * @param slot Mixer slot index (0-7).
   * @param name Descriptive name (e.g., "WavPlayer 0").
   */
  void setSourceName(uint8_t slot, const char *name);

  /**
   * @brief Retrieves the assigned name for a mixer slot.
   * @param slot Mixer slot index (0-7).
   * @return The source name string, or empty string if none assigned.
   */
  const char *getSourceName(uint8_t slot) const;

  /**
   * @brief Retrieves the source stream connected to a slot.
   */
  AudioStream *getSource(uint8_t slot) const {
    if (slot >= MAX_INPUTS)
      return nullptr;
    return sources_[slot];
  }

  /**
   * @brief Retrieves the channel on the source stream connected to a slot.
   */
  uint8_t getSourceChannel(uint8_t slot) const {
    if (slot >= MAX_INPUTS)
      return 0;
    return sourceChannels_[slot];
  }

protected:
  /**
   * @brief Main audio processing loop called by the Teensy Audio library.
   */
  void update() override;

private:
  audio_block_t
      *inputQueueArray[MAX_INPUTS]; ///< Required by AudioStream for buffering.

  AudioConnection
      *connections_[MAX_INPUTS]; ///< Pointers to managed patchcords.

  volatile int32_t gainsQ15_[MAX_INPUTS]; ///< Internal gains stored in Q15
                                          ///< fixed-point format.

  DuckingConfig duckingConfigs_[MAX_INPUTS]; ///< Ducking settings per channel.

  String sourceNames_[MAX_INPUTS]; ///< User-friendly labels for each input.

  AudioStream *sources_[MAX_INPUTS];   ///< Pointers to the original sources.
  uint8_t sourceChannels_[MAX_INPUTS]; ///< Source channel indices.

  /**
   * @brief Finds the first unoccupied mixer slot.
   * @return Index [0..7] or -1 if full.
   */
  int8_t findFreeSlot() const;

  /**
   * @brief Clamps a 32-bit integer to the 16-bit range of audio samples.
   */
  static inline int16_t clamp16(int32_t x) {
    if (x > 32767)
      return 32767;
    if (x < -32768)
      return -32768;
    return (int16_t)x;
  }

  /**
   * @brief Converts a floating point gain (0.0-1.0) to Q15 format.
   */
  static inline int32_t floatToQ15(float g) {
    if (g < 0.0f)
      g = 0.0f;
    if (g > 1.0f)
      g = 1.0f;
    return (int32_t)(g * 32767.0f + 0.5f);
  }

  /**
   * @brief Converts Q15 fixed-point gain back to floating point.
   */
  static inline float q15ToFloat(int32_t q) {
    if (q < 0)
      q = 0;
    if (q > 32767)
      q = 32767;
    return (float)q / 32767.0f;
  }
};