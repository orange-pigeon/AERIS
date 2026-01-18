#include <SD.h>
#include <SPI.h>

// Fallback defines for MTP buffers if not defined by the core
#ifndef MTP_TX_SIZE_480
#define MTP_TX_SIZE_480 512
#endif
#ifndef MTP_RX_SIZE_480
#define MTP_RX_SIZE_480 512
#endif

#include "AudioInputSlaveI2S2.h"
#include "Commander.h"
#include "Earcup.h"
#include "Mixer.h"
#include "RecorderManager.h"
#include "SerialHandler.h"
#include "WavPlayerManager.h"
#include <Arduino.h>
#include <Audio.h>
#include <MTP_Teensy.h>

#pragma region Hardware Configuration

AudioOutputI2S stereoAudioOutput;
AudioControlSGTL5000 audioShield;
#pragma endregion

#pragma region Component Initialization
Earcup earcupLeft; // Custom processing block for the left ear

// AudioInputI2S2 earcupInputLeft; // Microphone inputs (Outside Mic and Error
// Mic) - REPLACED BY BLUETOOTH
// Custom I2S2 Slave Input (Teensy is Slave, QCC is Master)
AudioAnalyzePeak btPeak; // Debug: Monitor signal level
Mixer earcupMixerLeft;   // Shared mixer for all sources going to the left

// System Managers
WavPlayerManager wavManager(earcupMixerLeft);
SinePlayerManager sineManager(earcupMixerLeft);
RecorderManager recorder;
Commander commander(earcupMixerLeft, wavManager, sineManager, recorder);
SerialHandler serialHandler(commander);

// MTP (MTP_Teensy uses the global MTP object)
#pragma endregion

#pragma region Audio Patching
// Microphones disabled for now as I2S2 is used for Bluetooth
// AudioConnection outsideMicInLeft(earcupInputLeft, 0, earcupLeft, 0);
// AudioConnection errorMicInLeft(earcupInputLeft, 1, earcupLeft, 1);

// Connect Bluetooth I2S2 directly to the mixer (Dynamic connection in setup
// preferred, but static here is fine if we remove the dynamic addInput in setup
// or use this as base) Actually, since we use Mixer::addInput in setup() for
// dynamic slot management/volume, we should NOT define static AudioConnections
// for the mixer inputs. The Mixer::addInput() method creates the connection
// dynamically.

// Connect Bluetooth I2S2 to Peak Analyzer for debugging
AudioConnection btPeakConn(qccAudio, 0, btPeak, 0);

// Connect the playback mixer to the earcup output path
AudioConnection audioInLeft(earcupMixerLeft, 0, earcupLeft, 2);

// Output the processed audio to the left speaker
AudioConnection audioOutLeft(earcupLeft, 0, stereoAudioOutput, 0);

#pragma endregion

#pragma region System Setup

// Diagnostic globals
uint32_t lastIsrCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to connect

  // Check for any breadcrumbs from a previous crash
  if (CrashReport) {
    Serial.println("--- CRASH DETECTED ---");
    Serial.print(CrashReport);
    Serial.println("----------------------");
    Serial.flush();
    delay(2000); // Let the user read it
  }

  // Allocate shared buffer memory for the Teensy Audio Library.
  // Increasing from 60 to 120 blocks to support simultaneous
  // playback, multi-source mixing, and recording.
  // Audio Memory: 20 is too low for complex chains. Increased to 120.
  AudioMemory(120);

  // Enable the SGTL5000 audio codec
  audioShield.enable();
  audioShield.volume(1.0f);

  // Initialize the SD card for WAV playback
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Error: SD initialization FAILED");
    while (true) {
      delay(1000); // Halt if SD card is missing
    }
  }

  if (!SD.exists("/recordings")) {
    SD.mkdir("/recordings");
  }

  if (!SD.exists("/config")) {
    SD.mkdir("/config");
  }

  // Register player sources with the mixer
  wavManager.init();
  sineManager.init();

  // Register Bluetooth Input
  qccAudio.begin();
  int8_t btSlotL = earcupMixerLeft.addInput(qccAudio, 0); // Left Channel
  if (btSlotL != -1) {
    earcupMixerLeft.setGain(btSlotL, 1.0f);
    earcupMixerLeft.setSourceName(btSlotL, "Bluetooth L");
    Serial.printf("Bluetooth routed to Left Earcup (Slot %d)\n", btSlotL);
  }

  // Initialize MTP and expose SD card as a disk
  MTP.addFilesystem(SD, "AERIS");
  MTP.begin();

  // Setup recorder sources

  // recorder.getMixer().addInput(earcupInputLeft, 0); // Slot 0: Mic 1
  // recorder.getMixer().setSourceName(0, "Mic1");
  // recorder.getMixer().addInput(earcupInputLeft, 1); // Slot 1: Mic 2
  // recorder.getMixer().setSourceName(1, "Mic2");
  // recorder.getMixer().addInput(earcupMixerLeft, 0); // Slot 2: System Output
  // recorder.getMixer().setSourceName(2, "System");

  // Default record gains (can be changed via serial)
  // recorder.getMixer().setGain(0, 1.0f);
  // recorder.getMixer().setGain(1, 1.0f);
  // recorder.getMixer().setGain(2, 0.5f); // Background recording of system
  // audio

  Serial.println("========================================");
  Serial.println("AERIS System Ready (I2S2 SLAVE MODE)");
  Serial.println("Wiring: Pin 3=LRCLK, Pin 4=BCLK, Pin 5=DATA");
  Serial.println("Type HELP for commands.");
  Serial.println("========================================");
}
#pragma endregion

#pragma region Main Loop

void loop() {
  // Check for incoming serial commands
  serialHandler.update();

  // Handle timed sine wave expiration
  sineManager.update();

  // Handle MTP background requests
  MTP.loop();

  // Handle background recording writes
  recorder.update();

  // Debug: Print Bluetooth Peak Level every 500ms
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 500) {
    // Debug data format: Print raw Hex values from the I2S Input Loop
    // We need to access the public buffer or use a volatile pointer?
    // Actually, let's just use the peak detector for now.
    // If sound is "bad", peak might be clipping or tiny.
    if (btPeak.available()) {
      Serial.printf("Peak: %.3f\n", btPeak.read());
    }

    // Debug: Check if ISR is running (Clocks receiving?)
    Serial.printf("ISR Count: %u\n", AudioInputSlaveI2S2::isrCount);

    // Debug: Calculate exact Sample Rate
    uint32_t diff = AudioInputSlaveI2S2::isrCount - lastIsrCount;
    // Each ISR = 128 samples (Half Buffer).
    // Interval is 500ms.
    // Samples/Sec = Diff * 128 * (1000 / 500) = Diff * 256
    float approxSampleRate = (float)diff * 256.0f;

    Serial.printf("ISR Diff: %u | Approx SR: %.0f Hz\n", diff,
                  approxSampleRate);
    lastIsrCount = AudioInputSlaveI2S2::isrCount;

    lastPrint = millis();
  }
}
#pragma endregion

#pragma endregion