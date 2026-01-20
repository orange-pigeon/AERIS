#include "Commander.h"
#include <MTP_Teensy.h>

Commander::Commander(Mixer &mixer, WavPlayer &wavManager,
                     SinePlayer &sineManager, RecorderManager &recorder)
    : mixer_(mixer), wavManager_(wavManager), sineManager_(sineManager),
      recorder_(recorder) {}

void Commander::handleCommand(const char *commandLine) {
  // Work on a copy of the command line since strtok modifies the string
  // in-place
  char buf[128];
  strncpy(buf, commandLine, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';

  char *cmd = strtok(buf, " ");
  if (!cmd)
    return;

  // Convert the command verb to uppercase for case-insensitive matching
  for (char *p = cmd; *p; ++p)
    *p = toupper(*p);

  char *args = strtok(NULL, ""); // Get the remaining arguments string

  // Command Dispatcher
  if (strcmp(cmd, "GAIN") == 0) {
    handleGain(args);
  } else if (strcmp(cmd, "DUCK") == 0) {
    handleDucking(args);
  } else if (strcmp(cmd, "PLAY") == 0 || strcmp(cmd, "PAUSE") == 0 ||
             strcmp(cmd, "STOP") == 0 || strcmp(cmd, "RESUME") == 0) {
    handlePlayer(cmd, args);
  } else if (strcmp(cmd, "SINE") == 0) {
    handleSine(args);
  } else if (strcmp(cmd, "REC") == 0) {
    handleRec(args);
  } else if (strcmp(cmd, "MTP") == 0) {
    handleMtp(args);
  } else if (strcmp(cmd, "SD") == 0) {
    handleSdInfo();
  } else if (strcmp(cmd, "SOURCES") == 0) {
    handleSources();
  } else if (strcmp(cmd, "HELP") == 0 || strcmp(cmd, "?") == 0) {
    printHelp();
  } else if (strcmp(cmd, "STATUS") == 0) {
    printStatus();
  } else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
  }
}

void Commander::handleGain(char *args) {
  if (!args) {
    Serial.println("Usage: GAIN <slot> <value>");
    return;
  }

  char *slotStr = strtok(args, " ");
  char *valStr = strtok(NULL, " ");

  if (!slotStr || !valStr) {
    Serial.println("Usage: GAIN <slot> <value>");
    return;
  }

  int slot = atoi(slotStr);
  float val = atof(valStr);

  mixer_.setGain((uint8_t)slot, val);
  Serial.printf("Set Gain Slot %d to %.2f\n", slot, val);
}

void Commander::handleDucking(char *args) {
  // Expected format: DUCK <target> <control> <gain> <thresh> <attack_ms>
  // <release_ms> Example: DUCK 0 4 0.1 0.05 20 1000

  if (!args) {
    Serial.println("Usage: DUCK <tgt> <ctrl> <gain> <thr> <att_ms> <rel_ms>");
    return;
  }

  char *tokens[6];
  tokens[0] = strtok(args, " ");
  for (int i = 1; i < 6; i++) {
    tokens[i] = strtok(NULL, " ");
    if (!tokens[i]) {
      Serial.println("Usage: DUCK <tgt> <ctrl> <gain> <thr> <att_ms> <rel_ms>");
      return;
    }
  }

  uint8_t target = (uint8_t)atoi(tokens[0]);
  int8_t control = (int8_t)atoi(tokens[1]);
  float gain = atof(tokens[2]);
  float thresh = atof(tokens[3]);
  float att = atof(tokens[4]);
  float rel = atof(tokens[5]);

  mixer_.setDucking(target, control, gain, thresh, att, rel);
  Serial.printf("Configured Ducking: Target=%d, Controlled by=%d, "
                "TargetGain=%.2f, Threshold=%.2f, Attk=%.0fms, Rel=%.0fms\n",
                target, control, gain, thresh, att, rel);
}

void Commander::handlePlayer(const char *cmd, char *args) {
  if (!args) {
    if (strcmp(cmd, "STOP") == 0) {
      Serial.println("Usage: STOP <index> or STOP ALL");
      return;
    }
    Serial.println(
        "Usage: PLAY <filename>, PAUSE <idx>, RESUME <idx>, STOP <idx>");
    return;
  }

  // Remove leading spaces
  while (*args == ' ')
    args++;

  if (strcmp(cmd, "PLAY") == 0) {
    wavManager_.play(args);
  } else if (strcmp(cmd, "STOP") == 0 && strcmp(args, "ALL") == 0) {
    wavManager_.stopAll();
  } else {
    // These sub-commands expect a numeric slot index
    int idx = atoi(args);

    if (strcmp(cmd, "PAUSE") == 0) {
      wavManager_.pause(idx);
    } else if (strcmp(cmd, "RESUME") == 0) {
      wavManager_.resume(idx);
    } else if (strcmp(cmd, "STOP") == 0) {
      wavManager_.stop(idx);
    }
  }
}

void Commander::printHelp() {
  Serial.println("Available commands:");
  Serial.println(
      "  GAIN <slot> <val>       : Set volume (0.0-1.0) for mixer slot (0-7)");
  Serial.println("  DUCK <tgt> <ctrl> <gain> <thr> <att> <rel> : Config "
                 "sidechain ducking");
  Serial.println(
      "  PLAY <filename>         : Start WAV file playback (assigns a slot)");
  Serial.println("  PAUSE <index>           : Pause WAV player at slot index");
  Serial.println("  RESUME <index>          : Resume WAV player at slot index");
  Serial.println(
      "  STOP <index>            : Stop player at slot index (or STOP ALL)");
  Serial.println(
      "  SINE <freq> <dur>       : Play sine tone (freq in Hz, dur in ms)");
  Serial.println(
      "  REC START <file>        : Start recording to /recordings/<file>");
  Serial.println("  REC STOP                : Stop and save recording");
  Serial.println(
      "  REC ADD SLOT <n>        : Add main mixer slot <n> to recording");
  Serial.println(
      "  REC CLEAR               : Remove all inputs from recording mixer");
  Serial.println("  MTP <ON|OFF>            : Toggle USB file transfer (MTP)");
  Serial.println("  SD                      : Show SD card status and usage");
  Serial.println("  SOURCES                 : List mixer inputs and assigned "
                 "device names");
  Serial.println("  STATUS                  : Show current gain settings");
}

void Commander::printStatus() {
  Serial.println("Mixer Channel Status:");
  for (int i = 0; i < mixer_.getNumSlots(); i++) {
    float g = mixer_.getGain(i);
    Serial.printf("  Slot %d: Gain=%.2f\n", i, g);
  }
}

void Commander::handleSine(char *args) {
  if (!args) {
    Serial.println("Usage: SINE <freq> <duration_ms>");
    return;
  }

  char *freqStr = strtok(args, " ");
  char *durStr = strtok(NULL, " ");

  if (!freqStr) {
    Serial.println("Usage: SINE <freq> <duration_ms>");
    return;
  }

  float freq = atof(freqStr);
  uint32_t dur = (durStr) ? (uint32_t)atoi(durStr) : 0;

  sineManager_.play(freq, dur);
}

void Commander::handleSources() {
  Serial.println("Mixer Inputs:");
  for (int i = 0; i < mixer_.getNumSlots(); i++) {
    const char *name = mixer_.getSourceName(i);
    float g = mixer_.getGain(i);
    if (strlen(name) > 0) {
      Serial.printf("  Slot %d: %s (Gain: %.2f)\n", i, name, g);
    } else {
      Serial.printf("  Slot %d: <Unassigned>\n", i);
    }
  }
}
void Commander::handleRec(char *args) {
  if (!args) {
    Serial.println("Usage: REC <START|STOP> [filename]");
    return;
  }

  char *subCmd = strtok(args, " ");
  if (strcmp(subCmd, "START") == 0) {
    char *filename = strtok(NULL, " ");
    if (!filename) {
      Serial.println("Error: No filename specified for REC START");
      return;
    }
    recorder_.start(filename);
  } else if (strcmp(subCmd, "STOP") == 0) {
    recorder_.stop();
  } else if (strcmp(subCmd, "ADD") == 0) {
    char *type = strtok(NULL, " ");
    if (type && strcmp(type, "SLOT") == 0) {
      char *slotStr = strtok(NULL, " ");
      if (slotStr) {
        int slotIdx = atoi(slotStr);
        AudioStream *src = mixer_.getSource(slotIdx);
        if (src) {
          uint8_t chan = mixer_.getSourceChannel(slotIdx);
          const char *name = mixer_.getSourceName(slotIdx);
          int recSlot = recorder_.getMixer().addInput(*src, chan);
          if (recSlot >= 0) {
            recorder_.getMixer().setSourceName(recSlot, name);
            Serial.printf("Added %s (Main Slot %d) to recorder slot %d.\n",
                          name, slotIdx, recSlot);
          } else {
            Serial.println("Error: Recorder mixer full.");
          }
        } else {
          Serial.printf("Error: No source at main mixer slot %d.\n", slotIdx);
        }
      } else {
        Serial.println("Usage: REC ADD SLOT <n>");
      }
    } else {
      Serial.println("Usage: REC ADD SLOT <n>");
    }
  } else if (strcmp(subCmd, "CLEAR") == 0) {
    recorder_.getMixer().clear();
    Serial.println("Recorder mixer cleared.");
  } else {
    Serial.println(
        "Error: Unknown REC command. Use START, STOP, ADD, or CLEAR.");
  }
}

void Commander::handleMtp(char *args) {
  if (!args) {
    Serial.println("Usage: MTP <ON|OFF>");
    return;
  }
  if (strcmp(args, "ON") == 0) {
    MTP.begin();
    Serial.println("MTP Enabled.");
  } else if (strcmp(args, "OFF") == 0) {
    Serial.println("MTP no longer serviced in main loop (Manual override).");
  } else {
    Serial.println("Usage: MTP <ON|OFF>");
  }
}

void Commander::handleSdInfo() {
  Serial.println("--- SD Card Status ---");
  if (!SD.exists("/")) {
    Serial.println("Status: NOT READABLE");
    return;
  }
  Serial.println("Status: ACTIVE");
  if (SD.exists("/recordings")) {
    Serial.println("Recordings folder: FOUND");
  } else {
    Serial.println("Recordings folder: MISSING");
  }
  Serial.println("----------------------");
}
