#include "WavPlayerManager.h" // Keep original header for this class
#include "Mixer.h"
#include <SD.h>

WavPlayerManager::WavPlayerManager(Mixer &outputMixer)
    : outputMixer_(outputMixer) {
  for (int i = 0; i < POOL_SIZE; i++) {
    filenames_[i] = "";
    mixerSlots_[i] = -1;
  }
}

void WavPlayerManager::init() {
  for (int i = 0; i < POOL_SIZE; i++) {
    // Register each player from the pool as an input to the mixer.
    // We store the resulting mixer slot ID for potential individual control
    // (e.g., ducking).
    mixerSlots_[i] = outputMixer_.addInput(players_[i], 0);
    if (mixerSlots_[i] != -1) {
      outputMixer_.setGain(mixerSlots_[i], 1.0f); // default volume
      char name[32];
      snprintf(name, sizeof(name), "WavPlayer %d", i);
      outputMixer_.setSourceName(mixerSlots_[i], name);
    } else {
      Serial.printf(
          "Error: Could not connect WavPlayer %d to Mixer (Slots full?)\n", i);
    }
  }
}

int WavPlayerManager::play(const char *filename) {
  // Construct full path if filename is not absolute
  String fullPath;
  if (filename[0] == '/') {
    fullPath = filename;
  } else {
    fullPath = String(BASE_DIR) + filename;
  }

  // Check if file even exists on the SD card before proceeding
  if (!SD.exists(fullPath.c_str())) {
    Serial.printf("Error: File %s not found on SD card\n", fullPath.c_str());
    return -1;
  }

  // Find an available player slot
  int slot = findFreePlayer();
  if (slot != -1) {
    filenames_[slot] = fullPath;
    players_[slot].setPause(
        false); // Ensure playback isn't starting in a paused state
    players_[slot].play(fullPath.c_str());
    Serial.printf("Started %s on slot %d\n", fullPath.c_str(), slot);
    return slot;
  } else {
    Serial.printf("No free players available for %s\n", fullPath.c_str());
    return -1;
  }
}

void WavPlayerManager::pause(int slot) {
  if (slot >= 0 && slot < POOL_SIZE) {
    players_[slot].setPause(true);
    Serial.printf("Paused slot %d\n", slot);
  } else {
    Serial.printf("Invalid player slot %d\n", slot);
  }
}

void WavPlayerManager::resume(int slot) {
  if (slot >= 0 && slot < POOL_SIZE) {
    players_[slot].setPause(false);
    Serial.printf("Resumed slot %d\n", slot);
  } else {
    Serial.printf("Invalid player slot %d\n", slot);
  }
}

void WavPlayerManager::stop(int slot) {
  if (slot >= 0 && slot < POOL_SIZE) {
    players_[slot].stop();
    players_[slot].setPause(false); // Clear pause flag for future use
    filenames_[slot] = "";          // Mark slot as free
    Serial.printf("Stopped slot %d\n", slot);
  } else {
    Serial.printf("Invalid player slot %d\n", slot);
  }
}

void WavPlayerManager::stopAll() {
  for (int i = 0; i < POOL_SIZE; i++) {
    if (players_[i].isPlaying() || players_[i].isPaused()) {
      players_[i].stop();
      players_[i].setPause(false);
    }
    filenames_[i] = "";
  }
  Serial.println("Stopped all WAV players");
}

int WavPlayerManager::findSlotFor(const char *filename) {
  String searchPath;
  if (filename[0] == '/') {
    searchPath = filename;
  } else {
    searchPath = String(BASE_DIR) + filename;
  }

  for (int i = 0; i < POOL_SIZE; i++) {
    if (filenames_[i].equalsIgnoreCase(searchPath)) {
      return i;
    }
  }
  return -1;
}

int WavPlayerManager::findFreePlayer() {
  // Seek the first player that is not currently busy.
  for (int i = 0; i < POOL_SIZE; i++) {
    if (!players_[i].isPlaying()) {
      return i;
    }
  }
  return -1;
}
