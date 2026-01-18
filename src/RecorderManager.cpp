#include "RecorderManager.h"

RecorderManager::RecorderManager()
    : patchCord_(recordingMixer_, 0, queue_, 0), recording_(false),
      byteCount_(0), bufferIndex_(0) {
  // We use the default internal buffer of AudioRecordQueue,
  // but we supplement it with a larger software buffer in update().
}

void RecorderManager::init() {
  // Default recording mixer gain to 1.0
  // No sources connected by default
}

bool RecorderManager::start(const char *filename) {
  if (recording_)
    stop();

  // Basic SD health check
  if (!SD.exists("/")) {
    Serial.println("Error: SD card seems to be disconnected or unreadable");
    return false;
  }

  String fullPath;
  if (filename[0] == '/') {
    fullPath = filename;
  } else {
    fullPath = String(BASE_DIR) + filename;
  }

  // Ensure directory exists (redundant but safe)
  if (!SD.exists(BASE_DIR)) {
    Serial.printf("Warning: %s folder missing, creating...\n", BASE_DIR);
    if (!SD.mkdir(BASE_DIR)) {
      Serial.println("Error: Could not create recordings directory");
      return false;
    }
  }

  // Delete existing file if it exists to ensure we start fresh
  if (SD.exists(fullPath.c_str())) {
    Serial.printf("Removing existing file: %s\n", fullPath.c_str());
    SD.remove(fullPath.c_str());
  }

  frec_ = SD.open(fullPath.c_str(), FILE_WRITE);
  if (frec_) {
    Serial.printf("Recording to %s...\n", fullPath.c_str());
    writeWavHeader(frec_);
    byteCount_ = 0;
    bufferIndex_ = 0; // Reset software buffer
    queue_.begin();
    recording_ = true;
    return true;
  } else {
    Serial.printf(
        "Error: Could not open %s for writing. (Is SD locked or full?)\n",
        fullPath.c_str());
    return false;
  }
}

void RecorderManager::stop() {
  if (!recording_)
    return;

  queue_.end();

  // Flush remaining data in the hardware queue to our software buffer
  while (queue_.available() > 0) {
    int16_t *buf = queue_.readBuffer();
    for (int i = 0; i < 128; i++) {
      if (bufferIndex_ < 16384) {
        recordBuffer_[bufferIndex_++] = buf[i];
      }
    }
    queue_.freeBuffer();
  }

  // Write any remaining data in the software buffer to SD
  if (bufferIndex_ > 0) {
    frec_.write((const uint8_t *)recordBuffer_, bufferIndex_ * 2);
    byteCount_ += (bufferIndex_ * 2);
    bufferIndex_ = 0;
  }

  finalizeWavHeader(frec_);
  frec_.close();
  recording_ = false;

  Serial.printf("Recording stopped. Total size: %u bytes\n", byteCount_);
}

void RecorderManager::update() {
  if (!recording_)
    return;

  // 1. Rapidly move data from hardware queue to software buffer.
  // This must be fast to avoid overflow during SD latency spikes.
  while (queue_.available() > 0) {
    int16_t *buf = queue_.readBuffer();
    // Copy 1 block (256 bytes / 128 samples)
    if (bufferIndex_ + 128 <= 16384) {
      memcpy(&recordBuffer_[bufferIndex_], buf, 256);
      bufferIndex_ += 128;
    } else {
      // Software Buffer Full!
      // If we hit this, the SD card is extremely slow.
    }
    queue_.freeBuffer();
  }

  // 2. Periodically flush software buffer to SD in large, efficient chunks.
  // We write when we have at least 8KB (4096 samples).
  if (bufferIndex_ >= 4096) {
    // Note: Writing in 8KB chunks helps with SD contention.
    frec_.write((const uint8_t *)recordBuffer_, 8192);
    byteCount_ += 8192;

    // Shift remaining data forward
    uint32_t samplesWritten = 4096;
    bufferIndex_ -= samplesWritten;
    if (bufferIndex_ > 0) {
      memmove(recordBuffer_, &recordBuffer_[samplesWritten], bufferIndex_ * 2);
    }

    // Explicitly flush to help the filesystem maintain state.
    frec_.flush();
  }
}

void RecorderManager::writeWavHeader(File &file) {
  file.write("RIFF", 4);
  uint32_t placeholder = 0;
  file.write((const uint8_t *)&placeholder, 4); // ChunkSize (File size - 8)
  file.write("WAVE", 4);

  file.write("fmt ", 4);
  uint32_t subchunk1Size = 16;
  file.write((const uint8_t *)&subchunk1Size, 4);
  uint16_t audioFormat = 1; // PCM
  file.write((const uint8_t *)&audioFormat, 2);
  uint16_t numChannels = 1; // Mono
  file.write((const uint8_t *)&numChannels, 2);
  uint32_t sampleRate = 44100;
  file.write((const uint8_t *)&sampleRate, 4);
  uint32_t byteRate = 44100 * 1 * 2;
  file.write((const uint8_t *)&byteRate, 4);
  uint16_t blockAlign = 1 * 2;
  file.write((const uint8_t *)&blockAlign, 2);
  uint16_t bitsPerSample = 16;
  file.write((const uint8_t *)&bitsPerSample, 2);

  file.write("data", 4);
  file.write((const uint8_t *)&placeholder, 4); // Subchunk2Size (Data size)
}

void RecorderManager::finalizeWavHeader(File &file) {
  uint32_t fileSize = byteCount_ + 36; // 44 - 8
  file.seek(4);
  file.write((const uint8_t *)&fileSize, 4);
  file.seek(40);
  file.write((const uint8_t *)&byteCount_, 4);
}
