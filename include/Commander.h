#pragma once

#include "Mixer.h"
#include "RecorderManager.h"
#include "SinePlayerManager.h"
#include "WavPlayerManager.h"
#include <Arduino.h>

/**
 * @class Commander
 * @brief Parses and dispatches serial commands to various system managers.
 *
 * This class acts as the central command processor, interpreting text-based
 * commands (like PLAY, GAIN, DUCK) and invoking the appropriate methods
 * on the Mixer, WavPlayerManager, and SinePlayerManager.
 */
class Commander {
public:
  /**
   * @brief Constructs the commander with references to necessary system
   * components.
   */
  Commander(Mixer &mixer, WavPlayerManager &wavManager,
            SinePlayerManager &sineManager, RecorderManager &recorder);

  /**
   * @brief Processes a single line of text into a command and arguments.
   * @param commandLine The raw string received via serial.
   */
  void handleCommand(const char *commandLine);

private:
  Mixer &mixer_;                   ///< Main audio mixer for gain and ducking.
  WavPlayerManager &wavManager_;   ///< Manager for WAV file playback.
  SinePlayerManager &sineManager_; ///< Manager for sine wave generation.
  RecorderManager &recorder_;      ///< Manager for audio recording.

  // Command handlers
  void handleGain(char *args);
  void handleDucking(char *args);
  void handlePlayer(const char *cmd, char *args);
  void handleSine(char *args);
  void handleRec(char *args);
  void handleMtp(char *args);
  void handleSdInfo();
  void handleSources();
  void printHelp();
  void printStatus();
};
