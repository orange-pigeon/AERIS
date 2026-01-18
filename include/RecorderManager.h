#pragma once

#include "Mixer.h"
#include <Arduino.h>
#include <Audio.h>
#include <SD.h>

/**
 * @class RecorderManager
 * @brief Manages audio recording to WAV files on the SD card.
 *
 * This manager provides a dedicated Mixer for recording, allowing multiple
 * sources to be blended and captured. It handles WAV header management
 * and background file writing.
 */
class RecorderManager {
public:
  RecorderManager();

  /**
   * @brief Initializes the recording components.
   */
  void init();

  /**
   * @brief Starts recording to a file in the /recordings directory.
   * @param filename Name of the file (e.g., "my_recording.wav").
   * @return True if recording started successfully.
   */
  bool start(const char *filename);

  /**
   * @brief Stops the current recording and finalizes the WAV header.
   */
  void stop();

  /**
   * @brief Processes the record queue and writes data to SD.
   * Should be called frequently in the main loop.
   */
  void update();

  /**
   * @brief Access the internal recording mixer to add/remove inputs.
   */
  Mixer &getMixer() { return recordingMixer_; }

  /**
   * @brief Returns true if a recording is currently in progress.
   */
  bool isRecording() const { return recording_; }

private:
  Mixer recordingMixer_;      ///< Mixer for blending sources to record.
  AudioRecordQueue queue_;    ///< Audio record queue for capturing samples.
  AudioConnection patchCord_; ///< Internal connection from mixer to queue.

  File frec_;      ///< Currently open file for recording.
  bool recording_; ///< State flag.

  uint32_t byteCount_;          ///< Tracks number of bytes written.
  int16_t recordBuffer_[16384]; ///< 32KB internal buffer (16384 samples).
  uint32_t bufferIndex_;        ///< Current position in the software buffer.

  /**
   * @brief Writes a placeholder WAV header to the start of the file.
   */
  void writeWavHeader(File &file);

  /**
   * @brief Updates the WAV header with the final chunk sizes.
   */
  void finalizeWavHeader(File &file);

  static constexpr const char *BASE_DIR = "/recordings/";
};
