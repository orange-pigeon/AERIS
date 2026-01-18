#include "Earcup.h"

Earcup::Earcup() : AudioStream(3, inputQueueArray) {}

// ---------- mic gains (kalibratie) ----------

void Earcup::setOutsideMicGain(float gain) {
  outsideMicGainQ15_ = floatToQ15(gain);
}
void Earcup::setErrorMicGain(float gain) {
  errorMicGainQ15_ = floatToQ15(gain);
}

float Earcup::getOutsideMicGain() const {
  return q15ToFloat(outsideMicGainQ15_);
}
float Earcup::getErrorMicGain() const { return q15ToFloat(errorMicGainQ15_); }

// ---------- mix levels ----------

void Earcup::setSidetoneLevel(float level) {
  sidetoneLevelQ15_ = floatToQ15(level);
}
float Earcup::getSidetoneLevel() const { return q15ToFloat(sidetoneLevelQ15_); }

// --- enable switches ---
void Earcup::setSidetoneEnabled(bool enabled) { sidetoneEnabled_ = enabled; }
bool Earcup::isSidetoneEnabled() const { return sidetoneEnabled_; }

void Earcup::update() {
  audio_block_t *outside = receiveReadOnly(0);
  audio_block_t *error = receiveReadOnly(1);
  audio_block_t *inMono = receiveReadOnly(2);

  // If we have no output buffer, we can't do anything.
  // But we must release inputs if they exist.
  audio_block_t *out = allocate();
  if (!out) {
    if (outside)
      release(outside);
    if (error)
      release(error);
    if (inMono)
      release(inMono);
    return;
  }

  // Prepare dummy silence buffers if inputs are missing
  // We cannot just pass NULL to process() because it expects valid arrays.
  // However, we can handle it inside process() or use a zeroed buffer here.
  // For efficiency, let's just handle specific cases or zero the output if
  // everything is null.

  // Better approach: Since process() does the math, we can just pass a
  // zero-filled array if the block is null. But allocating a zero block is
  // expensive. Instead, let's modify the process logic or, simpler: assume
  // inputs are present or safely skip them.

  // Actually, simpler fix:
  // If inMono is missing, there's no main audio. But maybe we want to hear
  // mics? If we want to support any combination, we need to be robust.

  // Let's rely on valid pointers.
  // If a block is NULL, we treat it as 0 data.
  // Getting a pointer to zeros is tricky without a block.
  // We can use the output block (temporarily cleared) as a source of zeros? No,
  // unsafe.

  // Quick Fix: If input 2 (Mixer) is missing, output silence (or just mic).
  // If Inputs 0/1 are missing, treat as silence.

  // For now, let's just ensure we have data for the math loop.
  int16_t zeros[AUDIO_BLOCK_SAMPLES];
  memset(zeros, 0, sizeof(zeros));

  const int16_t *pOutside = outside ? outside->data : zeros;
  const int16_t *pError = error ? error->data : zeros;
  const int16_t *pInMono = inMono ? inMono->data : zeros;

  process(out->data, pOutside, pError, pInMono, AUDIO_BLOCK_SAMPLES);

  transmit(out, 0);
  release(out);

  if (outside)
    release(outside);
  if (error)
    release(error);
  if (inMono)
    release(inMono);
}

void Earcup::process(int16_t *outAudio, const int16_t *outside,
                     const int16_t *error, const int16_t *inAudio, size_t n) {
  // snapshot
  const int32_t outMicG = outsideMicGainQ15_;
  const int32_t errMicG = errorMicGainQ15_;
  const int32_t sideLv = sidetoneLevelQ15_;
  const int32_t errLv = errorLevelQ15_;
  const bool sideOn = sidetoneEnabled_;

  // effectieve mix gains (Q15)
  const int32_t sideMixG = sideOn ? ((outMicG * sideLv) >> 15) : 0;
  const int32_t errMixG = (errMicG * errLv) >> 15;

  for (size_t i = 0; i < n; i++) {
    int32_t y = (int32_t)inAudio[i];

    // sidetone (hard uitgezet als sideMixG=0)
    y += (((int32_t)outside[i] * sideMixG) >> 15);

    // optioneel: error mic
    y += (((int32_t)error[i] * errMixG) >> 15);

    outAudio[i] = clamp16(y);
  }
}