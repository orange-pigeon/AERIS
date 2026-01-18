#include "SinePlayerManager.h"

SinePlayerManager::SinePlayerManager(Mixer &outputMixer)
    : outputMixer_(outputMixer) {
  for (int i = 0; i < POOL_SIZE; i++) {
    stopTimes_[i] = 0;
    active_[i] = false;
    mixerSlots_[i] = -1;
  }
}

void SinePlayerManager::init() {
  for (int i = 0; i < POOL_SIZE; i++) {
    // Add each generator as an input to the shared mixer.
    mixerSlots_[i] = outputMixer_.addInput(sines_[i], 0);
    if (mixerSlots_[i] != -1) {
      outputMixer_.setGain(mixerSlots_[i], 0.0f); // Start muted
      sines_[i].begin(0.0f, 0.0f, WAVEFORM_SINE); // Initialize as a sine wave
      char name[32];
      snprintf(name, sizeof(name), "SinePlayer %d", i);
      outputMixer_.setSourceName(mixerSlots_[i], name);
    } else {
      Serial.printf("Error: Could not connect SineWave generator %d to Mixer\n",
                    i);
    }
  }
}

void SinePlayerManager::play(float freq, uint32_t durationMs) {
  int i = findFreeSlot();
  if (i == -1) {
    Serial.println("Error: No free SinePlayer slots available");
    return;
  }

  // Configure the generator
  sines_[i].frequency(freq);
  sines_[i].amplitude(1.0f);

  // Calculate when to turn it off
  if (durationMs > 0) {
    stopTimes_[i] = millis() + durationMs;
  } else {
    stopTimes_[i] = 0; // Infinite playback until manual stop
  }

  active_[i] = true;
  outputMixer_.setGain(mixerSlots_[i],
                       0.5f); // Use half volume for the mixer slot for safety

  Serial.printf("Playing %.1f Hz for %u ms on sine slot %d\n", freq, durationMs,
                i);
}

void SinePlayerManager::stopAll() {
  for (int i = 0; i < POOL_SIZE; i++) {
    sines_[i].amplitude(0.0f);
    outputMixer_.setGain(mixerSlots_[i], 0.0f);
    active_[i] = false;
  }
}

void SinePlayerManager::update() {
  uint32_t now = millis();
  for (int i = 0; i < POOL_SIZE; i++) {
    // Check if an active timed tone has reached its expiration
    if (active_[i] && stopTimes_[i] > 0 && now >= stopTimes_[i]) {
      sines_[i].amplitude(0.0f);
      outputMixer_.setGain(mixerSlots_[i], 0.0f);
      active_[i] = false;
      Serial.printf("Sine slot %d duration expired\n", i);
    }
  }
}

int SinePlayerManager::findFreeSlot() {
  for (int i = 0; i < POOL_SIZE; i++) {
    if (!active_[i])
      return i;
  }
  return -1;
}
