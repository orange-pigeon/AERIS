#include <SD.h>
#include <SPI.h>

// Fallback defines for MTP buffers if not defined by the core
#ifndef MTP_TX_SIZE_480
#define MTP_TX_SIZE_480 512
#endif
#ifndef MTP_RX_SIZE_480
#define MTP_RX_SIZE_480 512
#endif

#include "Commander.h"
#include "Earcup.h"
#include "Mixer.h"
#include "RecorderManager.h"
#include "SerialHandler.h"
#include "SinePlayer.h"
#include "WavPlayer.h"

#include <Adafruit_PCM51xx.h>
#include <Arduino.h>
#include <Audio.h>
#include <MTP_Teensy.h>
#include <Wire.h>
#include <utility>

#pragma region Hardware Configuration

AudioOutputI2S stereoAudioOutput;
// AudioControlSGTL5000 audioShield;

#pragma endregion

#pragma region Component Initialization
Earcup earcupLeft; // Custom processing block for the left ear

AudioInputI2S2slave qccAudio;

Mixer earcupMixerLeft(10); // Standard 10 slots

WavPlayer wavManager(earcupMixerLeft);
SinePlayer sineManager(earcupMixerLeft);
RecorderManager recorder;

Commander commander(earcupMixerLeft, wavManager, sineManager, recorder);
SerialHandler serialHandler(commander);

Adafruit_PCM51xx pcm;

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

// Connect the playback mixer to the earcup output path
AudioConnection audioInLeft(earcupMixerLeft, 0, earcupLeft, 2);

// Output the processed audio to the left speaker
AudioConnection audioOutLeft(earcupLeft, 0, stereoAudioOutput, 0);

#pragma endregion

#pragma region System Setup

void setup()
{
  Serial.begin(115200);
  delay(1000); // Give serial time to connect

  // Check for any breadcrumbs from a previous crash
  if (CrashReport)
  {
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
  // audioShield.enable();
  // audioShield.volume(1.0f);

  // Initialize the SD card for WAV playback
  if (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("Error: SD initialization FAILED");
    while (true)
    {
      delay(1000); // Halt if SD card is missing
    }
  }

  if (!SD.exists("/recordings"))
  {
    SD.mkdir("/recordings");
  }

  if (!SD.exists("/config"))
  {
    SD.mkdir("/config");
  }

  wavManager.setMixerSlots({0, 1, 2, 3});
  wavManager.init();

  sineManager.setMixerSlots({4, 5, 6, 7});
  sineManager.init();

  // Register Bluetooth Input
  qccAudio.begin();
  int8_t btSlotL = earcupMixerLeft.addInput(qccAudio, 0); // Left Channel
  if (btSlotL != -1)
  {
    earcupMixerLeft.setGain(btSlotL, 1.0f);
    earcupMixerLeft.setSourceName(btSlotL, "Bluetooth L");
    Serial.printf("Bluetooth routed to Left Earcup (Slot %d)\n", btSlotL);

    // --- Ducking Configuration ---
    // Target: btSlotL (Bluetooth, which is Slot 8)
    // Control: Slot 9 (Summed WAVs from wavSumMixer)
    // Settings: 10% volume, 0.05 threshold, 100ms attack, 800ms release
    earcupMixerLeft.setDucking(btSlotL, 9, 0.10f, 0.05f, 1000.0f, 1000.0f);
    earcupMixerLeft.setGain(9, 0.0f); // Level monitoring only
  }

  // Initialize MTP and expose SD card as a disk
  MTP.addFilesystem(SD, "AERIS");
  MTP.begin();

  pinMode(36, OUTPUT);
  digitalWrite(36, HIGH);

  if (!pcm.begin())
  {
    Serial.println(F("Could not find PCM51xx, check wiring!"));
    while (1)
      delay(100);
  }
  pcm.setI2SFormat(PCM51XX_I2S_FORMAT_I2S);
  // Read and display current format
  pcm51xx_i2s_format_t format = pcm.getI2SFormat();
  Serial.print(F("Current I2S format: "));
  switch (format)
  {
  case PCM51XX_I2S_FORMAT_I2S:
    Serial.println(F("I2S"));
    break;
  case PCM51XX_I2S_FORMAT_TDM:
    Serial.println(F("TDM/DSP"));
    break;
  case PCM51XX_I2S_FORMAT_RTJ:
    Serial.println(F("Right Justified"));
    break;
  case PCM51XX_I2S_FORMAT_LTJ:
    Serial.println(F("Left Justified"));
    break;
  default:
    Serial.println(F("Unknown"));
    break;
  }
  // Set I2S word length to 32-bit
  Serial.println(F("Setting I2S word length"));
  pcm.setI2SSize(PCM51XX_I2S_SIZE_16BIT);

  // Read and display current word length
  pcm51xx_i2s_size_t size = pcm.getI2SSize();
  Serial.print(F("Current I2S word length: "));
  switch (size)
  {
  case PCM51XX_I2S_SIZE_16BIT:
    Serial.println(F("16 bits"));
    break;
  case PCM51XX_I2S_SIZE_20BIT:
    Serial.println(F("20 bits"));
    break;
  case PCM51XX_I2S_SIZE_24BIT:
    Serial.println(F("24 bits"));
    break;
  case PCM51XX_I2S_SIZE_32BIT:
    Serial.println(F("32 bits"));
    break;
  default:
    Serial.println(F("Unknown"));
    break;
  }

  // Set error detection bits
  if (!pcm.ignoreFSDetect(true) || !pcm.ignoreBCKDetect(true) ||
      !pcm.ignoreSCKDetect(true) || !pcm.ignoreClockHalt(true) ||
      !pcm.ignoreClockMissing(true) || !pcm.disableClockAutoset(false) ||
      !pcm.ignorePLLUnlock(true))
  {
    Serial.println(F("Error detection failed to configure"));
  }

  // Enable PLL
  Serial.println(F("Enabling PLL"));
  pcm.enablePLL(true);

  // Check PLL status
  bool pllEnabled = pcm.isPLLEnabled();
  Serial.print(F("PLL enabled: "));
  Serial.println(pllEnabled ? F("Yes") : F("No"));

  // Set PLL reference to BCK
  Serial.println(F("Setting PLL reference"));
  pcm.setPLLReference(PCM51XX_PLL_REF_BCK);

  // Read and display current PLL reference
  pcm51xx_pll_ref_t pllRef = pcm.getPLLReference();
  Serial.print(F("Current PLL reference: "));
  switch (pllRef)
  {
  case PCM51XX_PLL_REF_SCK:
    Serial.println(F("SCK"));
    break;
  case PCM51XX_PLL_REF_BCK:
    Serial.println(F("BCK"));
    break;
  case PCM51XX_PLL_REF_GPIO:
    Serial.println(F("GPIO"));
    break;
  default:
    Serial.println(F("Unknown"));
    break;
  }

  // Set DAC clock source to PLL
  Serial.println(F("Setting DAC source"));
  pcm.setDACSource(PCM51XX_DAC_CLK_PLL);

  // Read and display current DAC source
  pcm51xx_dac_clk_src_t dacSource = pcm.getDACSource();
  Serial.print(F("Current DAC source: "));
  switch (dacSource)
  {
  case PCM51XX_DAC_CLK_MASTER:
    Serial.println(F("Master clock (auto-select)"));
    break;
  case PCM51XX_DAC_CLK_PLL:
    Serial.println(F("PLL clock"));
    break;
  case PCM51XX_DAC_CLK_SCK:
    Serial.println(F("SCK clock"));
    break;
  case PCM51XX_DAC_CLK_BCK:
    Serial.println(F("BCK clock"));
    break;
  default:
    Serial.println(F("Unknown"));
    break;
  }

  // Test auto mute (default turn off)
  Serial.println(F("Setting auto mute"));
  pcm.setAutoMute(false);

  // Read and display current auto mute status
  bool autoMuteEnabled = pcm.getAutoMute();
  Serial.print(F("Auto mute: "));
  Serial.println(autoMuteEnabled ? F("Enabled") : F("Disabled"));

  // Test mute (default do not mute)
  Serial.println(F("Setting mute"));
  pcm.mute(false);

  // Read and display current mute status
  bool muteEnabled = pcm.isMuted();
  Serial.print(F("Mute: "));
  Serial.println(muteEnabled ? F("Enabled") : F("Disabled"));

  // Check DSP boot status and power state
  Serial.print(F("DSP boot done: "));
  Serial.println(pcm.getDSPBootDone() ? F("Yes") : F("No"));

  pcm51xx_power_state_t powerState = pcm.getPowerState();
  Serial.print(F("Power state: "));
  switch (powerState)
  {
  case PCM51XX_POWER_POWERDOWN:
    Serial.println(F("Powerdown"));
    break;
  case PCM51XX_POWER_WAIT_CP_VALID:
    Serial.println(F("Wait for CP voltage valid"));
    break;
  case PCM51XX_POWER_CALIBRATION_1:
  case PCM51XX_POWER_CALIBRATION_2:
    Serial.println(F("Calibration"));
    break;
  case PCM51XX_POWER_VOLUME_RAMP_UP:
    Serial.println(F("Volume ramp up"));
    break;
  case PCM51XX_POWER_RUN_PLAYING:
    Serial.println(F("Run (Playing)"));
    break;
  case PCM51XX_POWER_LINE_SHORT:
    Serial.println(F("Line output short / Low impedance"));
    break;
  case PCM51XX_POWER_VOLUME_RAMP_DOWN:
    Serial.println(F("Volume ramp down"));
    break;
  case PCM51XX_POWER_STANDBY:
    Serial.println(F("Standby"));
    break;
  default:
    Serial.println(F("Unknown"));
    break;
  }

  // Check PLL lock status
  bool pllLocked = pcm.isPLLLocked();
  Serial.print(F("PLL locked: "));
  Serial.println(pllLocked ? F("Yes") : F("No"));

  // Set volume to -6dB on both channels
  Serial.println(F("Setting volume"));
  pcm.setVolumeDB(-10.0, -10.0);

  // Read and display current volume
  float leftVol, rightVol;
  pcm.getVolumeDB(&leftVol, &rightVol);
  Serial.print(F("Current volume - Left: "));
  Serial.print(leftVol, 1);
  Serial.print(F("dB, Right: "));
  Serial.print(rightVol, 1);
  Serial.println(F("dB"));

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
}
#pragma endregion

#pragma region Main Loop

void loop()
{
  serialHandler.update();
  recorder.update();
  MTP.loop();
}
#pragma endregion

#pragma endregion