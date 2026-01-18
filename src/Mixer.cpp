#include "Mixer.h"

Mixer::Mixer() : AudioStream(MAX_INPUTS, inputQueueArray) {
  for (uint8_t i = 0; i < MAX_INPUTS; i++) {
    connections_[i] = nullptr;
    sources_[i] = nullptr;
    sourceChannels_[i] = 0;
    gainsQ15_[i] = 0;

    // Initialize ducking configuration with disabled/default states
    duckingConfigs_[i].controlSlot = -1;
    duckingConfigs_[i].duckingGain = 1.0f;
    duckingConfigs_[i].threshold = 0.0f;
    duckingConfigs_[i].attack = 0.1f;
    duckingConfigs_[i].release = 0.1f;
    duckingConfigs_[i].currentScale = 1.0f;
    sourceNames_[i] = "";
  }
}

int8_t Mixer::findFreeSlot() const {
  for (uint8_t i = 0; i < MAX_INPUTS; i++) {
    if (connections_[i] == nullptr)
      return (int8_t)i;
  }
  return -1;
}

int8_t Mixer::addInput(AudioStream &source, uint8_t sourceChannel) {
  int8_t slot = findFreeSlot();
  if (slot < 0)
    return -1; // No empty slots available

  // Dynamically create a connection between the source and this mixer.
  // We must disable audio interrupts to avoid crashing if the update
  // loop runs while we are half-way through creating the connection.
  AudioNoInterrupts();
  connections_[slot] =
      new AudioConnection(source, sourceChannel, *this, (uint8_t)slot);

  sources_[slot] = &source;
  sourceChannels_[slot] = sourceChannel;
  AudioInterrupts();

  gainsQ15_[slot] = floatToQ15(1.0f); // Default to full volume
  return slot;
}

void Mixer::removeInput(uint8_t slot) {
  if (slot >= MAX_INPUTS)
    return;

  AudioNoInterrupts();
  if (connections_[slot]) {
    delete connections_[slot];
    connections_[slot] = nullptr;
  }
  sources_[slot] = nullptr;
  sourceChannels_[slot] = 0;
  AudioInterrupts();

  gainsQ15_[slot] = 0;     // Reset gain
  sourceNames_[slot] = ""; // Reset assigned name
}

void Mixer::clear() {
  for (uint8_t i = 0; i < MAX_INPUTS; i++) {
    removeInput(i);
  }
}

void Mixer::setGain(uint8_t slot, float gain) {
  if (slot >= MAX_INPUTS)
    return;
  gainsQ15_[slot] = floatToQ15(gain);
}

float Mixer::getGain(uint8_t slot) const {
  if (slot >= MAX_INPUTS)
    return 0.0f;
  return q15ToFloat(gainsQ15_[slot]);
}

void Mixer::setDucking(uint8_t targetSlot, int8_t controlSlot, float duckGain,
                       float threshold, float attackMs, float releaseMs) {
  if (targetSlot >= MAX_INPUTS)
    return;

  // Approximate milliseconds per audio block (at 44.1kHz, 128 samples is
  // ~2.9ms)
  float dt = 2.9f;

  // Ensure fade times aren't faster than a single block
  if (attackMs < dt)
    attackMs = dt;
  if (releaseMs < dt)
    releaseMs = dt;

  duckingConfigs_[targetSlot].controlSlot = controlSlot;
  duckingConfigs_[targetSlot].duckingGain = duckGain;
  duckingConfigs_[targetSlot].threshold = threshold;

  // Calculate incremental changes per audio update call
  duckingConfigs_[targetSlot].release = 1.0f / (releaseMs / dt);
  duckingConfigs_[targetSlot].attack = 1.0f / (attackMs / dt);
}

void Mixer::update() {
  audio_block_t *inBlocks[MAX_INPUTS];
  bool any = false;

  // Fetch audio data from all connected inputs
  for (uint8_t i = 0; i < MAX_INPUTS; i++) {
    if (connections_[i] == nullptr) {
      inBlocks[i] = nullptr;
      continue;
    }

    inBlocks[i] = receiveReadOnly(i);
    if (inBlocks[i])
      any = true;
  }

  // If no audio blocks were received, there's nothing to mix
  if (!any) {
    return;
  }

  // --- Step 1: Process Ducking Envelopes ---
  for (uint8_t i = 0; i < MAX_INPUTS; i++) {
    DuckingConfig &cfg = duckingConfigs_[i];

    // If ducking is configured for this channel
    if (cfg.controlSlot >= 0 && cfg.controlSlot < MAX_INPUTS) {
      audio_block_t *ctrlBlock = inBlocks[cfg.controlSlot];
      float peak = 0.0f;

      // Extract peak amplitude from the controlling channel
      if (ctrlBlock) {
        for (int s = 0; s < AUDIO_BLOCK_SAMPLES;
             s += 8) { // Sample every 8th value for performance
          int16_t val = ctrlBlock->data[s];
          if (val < 0)
            val = -val;
          if (val > (int16_t)(peak * 32767.0f)) {
            peak = (float)val / 32767.0f;
          }
        }
      }

      // Determine target gain (ducked or normal)
      float targetScale = 1.0f;
      if (peak > cfg.threshold) {
        targetScale = cfg.duckingGain;
      }

      // Smoothly transition the current scaling factor
      if (cfg.currentScale > targetScale) {
        cfg.currentScale -= cfg.attack; // Transition down (Attack)
        if (cfg.currentScale < targetScale)
          cfg.currentScale = targetScale;
      } else if (cfg.currentScale < targetScale) {
        cfg.currentScale += cfg.release; // Transition up (Release)
        if (cfg.currentScale > targetScale)
          cfg.currentScale = targetScale;
      }
    } else {
      // Default to no reduction if no control slot is set
      cfg.currentScale = 1.0f;
    }
  }

  // --- Step 2: Mixing ---
  audio_block_t *out = allocate();
  if (!out) {
    // Allocation failed, clean up received blocks
    for (uint8_t i = 0; i < MAX_INPUTS; i++) {
      if (inBlocks[i])
        release(inBlocks[i]);
    }
    return;
  }

  // Initialize output buffer with silence
  memset(out->data, 0, sizeof(out->data));

  for (uint8_t i = 0; i < MAX_INPUTS; i++) {
    audio_block_t *b = inBlocks[i];

    // Skip if no block or gain is zero
    if (!b || gainsQ15_[i] == 0) {
      if (b)
        release(b);
      continue;
    }

    float scale = duckingConfigs_[i].currentScale;
    int32_t effGain = gainsQ15_[i];

    // Apply ducking attenuation if active
    if (scale < 1.0f) {
      effGain = (int32_t)(effGain * scale);
    }

    // Accumulate weighted samples into output buffer
    for (uint16_t s = 0; s < AUDIO_BLOCK_SAMPLES; s++) {
      int32_t acc = out->data[s];
      acc += (((int32_t)b->data[s] * effGain) >> 15);
      out->data[s] = clamp16(acc); // Prevent clipping
    }
    release(b);
  }

  // Step 3: Transmit Mixed Signal
  transmit(out, 0);
  release(out);
}

void Mixer::setSourceName(uint8_t slot, const char *name) {
  if (slot < MAX_INPUTS) {
    sourceNames_[slot] = String(name);
  }
}

const char *Mixer::getSourceName(uint8_t slot) const {
  if (slot < MAX_INPUTS) {
    return sourceNames_[slot].c_str();
  }
  return "";
}