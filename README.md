# AERIS Audio Platform

A professional, 8-channel audio mixing and playback system designed for the **Teensy 4.1** using the **Teensy Audio Library**. AERIS features a custom side-chain ducking mixer, concurrent WAV playback from SD, and real-time serial command control.

---

## 🚀 Features

- **8-Channel Custom Mixer**: High-performance mixing with individual gain controls.
- **Side-Chain Ducking**: Automate volume attenuation on any channel triggered by another (e.g., lower music when voice is active).
- **Concurrent WAV Playback**: Pool of 4 independent players for playing multiple files from SD simultaneously.
- **Timed Sine Generation**: Pool of 4 sine wave generators with precise frequency and duration control.
- **Source Naming**: Assign human-readable labels to mixer slots for easy identification.
- **Serial Interface**: Complete CLI for system configuration and real-time playback control.

---

## 🛠 Hardware Required

- **Teensy 4.1**
- **Teensy Audio Shield (SGTL5000)**
- **SD Card** (containing `.wav` files at 44.1kHz, 16-bit mono/stereo)
- **Audio Output**: 3.5mm jack or speakers connected to the shield.

---

## ⌨️ Serial Command Interface

Connect via any serial monitor at **115200 baud**. Commands are case-insensitive.

### **Playback Commands**

| Command | Usage | Description |
| :--- | :--- | :--- |
| `PLAY` | `PLAY <filename>` | Starts a file (e.g., `1.wav`). It returns the assigned slot index. |
| `PAUSE` | `PAUSE <index>` | Pauses playback on the specified WAV slot (0-3). |
| `RESUME`| `RESUME <index>` | Resumes a specifically paused WAV slot. |
| `STOP`  | `STOP <index>` | Stops a specific WAV slot. |
| `STOP`  | `STOP ALL` | Stops all active WAV players. |

### **Signal Generation**

| Command | Usage | Description |
| :--- | :--- | :--- |
| `SINE` | `SINE <freq> <dur>` | Plays a tone (e.g., `SINE 440 1000` for 1s of A4). `dur=0` for infinite. |

### **Mixer & Ducking**

| Command | Usage | Description |
| :--- | :--- | :--- |
| `GAIN` | `GAIN <slot> <val>`| Set slot volume (0.0 to 1.0). Mixer slots are 0-7. |
| `SOURCES`| `SOURCES` | List all mixer slots and what device is assigned to them. |
| `DUCK` | `DUCK <tgt> <ctrl> <gain> <thr> <att> <rel>` | Configure side-chain ducking. |

**Ducking Example:**
`DUCK 0 4 0.1 0.05 20 1000`
- **Target (0)**: The channel to be lowered (e.g., music).
- **Control (4)**: The channel that triggers the drop (e.g., a test tone).
- **Gain (0.1)**: Volume drops to 10% when triggered.
- **Threshold (0.05)**: Sensitivity of the trigger.
- **Attack/Release (20/1000)**: Speed of fade-down and fade-up in milliseconds.

---

## 📂 Project Structure

- `src/main.cpp`: System entry point and audio patching.
- `src/Mixer.cpp`: Core mixing logic and side-chain ducking implementation.
- `src/WavPlayerManager.cpp`: Manages the pool of 4 SD players.
- `src/SinePlayerManager.cpp`: Manages the pool of 4 sine generators.
- `src/Commander.cpp`: Command parser and dispatcher.
- `src/SerialHandler.cpp`: Line-buffered serial communication.

---

## 📋 Getting Started

1.  **Format SD**: Ensure your SD card is FAT32.
2.  **Add Files**: Place `.wav` files in the root directory.
3.  **Upload**: Flash the firmware using PlatformIO or Arduino IDE.
4.  **Monitor**: Open the Serial Monitor and type `HELP` to begin.
