#include "WavPlayer.h"
#include "Mixer.h"
#include <SD.h>

WavPlayer::WavPlayer(Mixer &outputMixer) : outputMixer_(outputMixer) {
  for (int i = 0; i < POOL_SIZE; i++) {
    filenames_[i] = "";
    mixerSlots_[i] = -1;
    requestedSlots_[i] = -1;
  }
}

void WavPlayer::init() {
  for (int i = 0; i < POOL_SIZE; i++) {
    // Register each player from the pool as an input to the mixer.
    // We check if a specific slot was requested; otherwise we add to next free
    // slot.
    int8_t targetSlot = requestedSlots_[i];

    if (targetSlot != -1) {
      if (outputMixer_.setInput(targetSlot, players_[i], 0)) {
        mixerSlots_[i] = targetSlot;
      } else {
        Serial.printf(
            "Error: Could not connect WavPlayer %d to requested Mixer "
            "Slot %d\n",
            i, targetSlot);
        mixerSlots_[i] = -1;
      }
    } else {
      mixerSlots_[i] = outputMixer_.addInput(players_[i], 0);
    }

    if (mixerSlots_[i] != -1) {
      outputMixer_.setGain(mixerSlots_[i], 1.0f); // default volume
      char name[32];
      snprintf(name, sizeof(name), "WavPlayer %d", i);
      outputMixer_.setSourceName(mixerSlots_[i], name);
    } else if (targetSlot == -1) {
      Serial.printf(
          "Error: Could not connect WavPlayer %d to Mixer (Slots full?)\n", i);
    }
  }
}

void WavPlayer::setMixerSlots(std::initializer_list<int8_t> slots) {
  int i = 0;
  for (int8_t val : slots) {
    if (i < POOL_SIZE) {
      requestedSlots_[i++] = val;
    }
  }
}

int WavPlayer::play(const char *filename) {
  // Construct full path if filename is not absolute
  String fullPath;
  if (filename[0] == '/') {
    fullPath = filename;
  } else {
    fullPath = String(BASE_DIR) + filename;
  }

  // Find an available player slot
  int slot = findFreePlayer();
  if (slot == -1) {
    Serial.print("No free players available for the WAV player.\n");
    return -1;
  }

  filenames_[slot] = fullPath;
  players_[slot].setPause(false);

  if (players_[slot].play(fullPath.c_str())) {
    Serial.printf("Started %s on slot %d\n", filename, slot);
    return slot;
  } else {
    Serial.printf(
        "Error: Failed to play %s on slot %d (SD error or file missing)\n",
        fullPath.c_str(), slot);
    filenames_[slot] = "";
    return -1;
  }
}

void WavPlayer::pause(int slot) {
  if (slot >= 0 && slot < POOL_SIZE) {
    players_[slot].setPause(true);
    Serial.printf("Paused slot %d\n", slot);
  } else {
    Serial.printf("Invalid player slot %d\n", slot);
  }
}

void WavPlayer::resume(int slot) {
  if (slot >= 0 && slot < POOL_SIZE) {
    players_[slot].setPause(false);
    Serial.printf("Resumed slot %d\n", slot);
  } else {
    Serial.printf("Invalid player slot %d\n", slot);
  }
}

void WavPlayer::stop(int slot) {
  if (slot >= 0 && slot < POOL_SIZE) {
    players_[slot].stop();
    players_[slot].setPause(false); // Clear pause flag for future use
    filenames_[slot] = "";          // Mark slot as free
    Serial.printf("Stopped slot %d\n", slot);
  } else {
    Serial.printf("Invalid player slot %d\n", slot);
  }
}

void WavPlayer::stopAll() {
  for (int i = 0; i < POOL_SIZE; i++) {
    if (players_[i].isPlaying() || players_[i].isPaused()) {
      players_[i].stop();
      players_[i].setPause(false);
    }
    filenames_[i] = "";
  }
  Serial.println("Stopped all WAV players");
}

int WavPlayer::findSlotFor(const char *filename) {
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

int WavPlayer::findFreePlayer() {
  // Seek the first player that is not currently busy.
  for (int i = 0; i < POOL_SIZE; i++) {
    if (!players_[i].isPlaying()) {
      return i;
    }
  }
  return -1;
}
