#pragma once

#include "Mixer.h"
#include <Arduino.h>
#include <Audio.h>
#include <initializer_list>

/**
 * @class PausableAudioPlaySdWav
 * @brief Extends the standard AudioPlaySdWav to provide explicit pause control.
 *
 * The standard library's pause mechanism can be inconsistent; this subclass
 * ensures the update loop explicitly skips processing when paused.
 */
class PausableAudioPlaySdWav : public AudioPlaySdWav {
public:
  bool _paused = false;

  void setPause(bool p) { _paused = p; }
  void togglePause() { _paused = !_paused; }
  bool isPaused() { return _paused; }

protected:
  virtual void update() override {
    if (_paused)
      return; // Transmits no data, effectively pausing the stream and file
              // read.
    AudioPlaySdWav::update();
  }
};

/**
 * @class WavPlayer
 * @brief Manages a pool of WAV players to allow concurrent playback and easy
 * control.
 *
 * This manager handles file selection, player assignment, and provides an
 * index-based interface for external commands.
 */
class WavPlayer {
public:
  static const int POOL_SIZE = 4; ///< Number of concurrent WAV players.
  static constexpr const char *BASE_DIR =
      "/recordings/"; ///< Base directory for WAV files.

  WavPlayer(Mixer &outputMixer);

  /**
   * @brief Initializes players and registers them with the output mixer.
   */
  void init();

  /**
   * @brief Configures which mixer slots this manager should use.
   * Should be called before init().
   * @param slots An initializer list of slot indices (e.g., {0, 1, 2, 3}). Use
   * -1 for dynamic assignment.
   */
  void setMixerSlots(std::initializer_list<int8_t> slots);

  /**
   * @brief Starts playback of a WAV file on the first available player.
   * @param filename Path to the file on the SD card (e.g., "1.wav").
   * @return The assigned slot index (0 to POOL_SIZE-1) or -1 if no players are
   * free.
   */
  int play(const char *filename);

  /**
   * @brief Pauses a specific player slot.
   */
  void pause(int slot);

  /**
   * @brief Resumes a specifically paused player slot.
   */
  void resume(int slot);

  /**
   * @brief Stops playback on a specific slot and frees it.
   */
  void stop(int slot);

  /**
   * @brief Stops all active WAV playback.
   */
  void stopAll();

  /**
   * @brief Returns a reference to a specific player object.
   */
  PausableAudioPlaySdWav &getPlayer(int slot) { return players_[slot]; }

private:
  Mixer &outputMixer_; ///< Reference to the main audio mixer.
  PausableAudioPlaySdWav
      players_[POOL_SIZE];      ///< Pool of independent player objects.
  String filenames_[POOL_SIZE]; ///< Tracks which file is playing in each slot.
  int8_t
      mixerSlots_[POOL_SIZE]; ///< Maps manager slot index to mixer input index.
  int8_t requestedSlots_[POOL_SIZE]; ///< User-requested mixer slots (-1 =
                                     ///< dynamic).

  /**
   * @brief Finds the first slot currently playing a specific file.
   */
  int findSlotFor(const char *filename);

  /**
   * @brief Finds the first player that is not currently active.
   */
  int findFreePlayer();
};
