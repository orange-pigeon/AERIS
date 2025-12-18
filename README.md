# ✈️ AERIS  
**High-Fidelity General Aviation Headset – Proof of Concept**

<div align="center">
  <img width="911" height="820" alt="OP-AERIS" src="https://github.com/user-attachments/assets/33ea24da-07fe-4eed-9816-3114d598dea0" />
</div>

---

## 🧭 Overview
**AERIS** is a Proof-of-Concept (PoC) project focused on developing a **premium General Aviation (GA) headset**.  
The goal is to combine **audiophile-grade sound quality** with **aviation-specific functionality**, delivering an exceptional cockpit experience.

Key focus areas include:
- High-performance **Active Noise Cancellation**
- Seamless **wireless connectivity**
- Integrated **ATC recording & playback**

---

## ✨ Key Features

### 🎧 Active Noise Cancellation (ANC)
- Advanced **hybrid ANC** architecture  
- Feedforward + feedback microphones per earcup  
- Optimized for continuous low-frequency cockpit noise

### 📡 Bluetooth Connectivity
- 🎵 **Music Streaming (A2DP)**
- 📞 **Telephony (HFP)** – hands-free calling
- 🔄 **OTA Firmware Updates**
- 📥 **ATC Audio Downloads** to mobile devices

### 🎙️ ATC Recording
- Direct recording of ATC communications  
- High-quality onboard storage for later review and analysis

---

## 🧩 Hardware Specifications
AERIS is built around **high-end audio components** to ensure clarity, low distortion, and effective noise suppression.

### 🔊 Audio Drivers
- **Model**: AH-D9200  
- **Type**: High-Fidelity Dynamic Drivers  
- **Description**:  
  Sourced from premium audiophile headphones to provide excellent dynamic range and intelligibility for both speech and media.

### 🎤 Microphone Configuration

#### ANC Microphones
- **Sensor**: ICS-43434 (MEMS)
- **Configuration**: 4× total (2 per earcup)
- **Placement**:
  - 1× External (Feedforward)
  - 1× Internal (Feedback)

#### Communications Microphone
- **Model**: PA-9EHN  
- **Type**: Analog Electret  
- **Purpose**:  
  Aviation-grade noise-canceling pilot microphone optimized for high-noise environments.

---

## 🧪 Test Phase Setup
To validate the AERIS concept and fine-tune the audio processing chain, a dedicated **test platform** is currently in use.

### 🪖 Acoustic Platform
- **Chassis**: **3M™ PELTOR™ Optime™ III** earmuffs  
- **Rationale**:  
  Excellent passive noise attenuation provides a stable baseline for ANC evaluation.

### ⚙️ Electronics & Audio Components
- 🧠 **Core MCU**: Teensy 4.1  
  - High-performance microcontroller for low-latency DSP
- 🎚️ **Audio Interface**: Teensy Audio Adaptor Board  
  - Integrated CODEC and audio I/O
- 🔈 **Speakers**: 4070 Speaker (4Ω, 3W)  
- 🎙️ **Test Microphones**:  
  - 2× INMP441 MEMS microphones (I2S)
- 📶 **Wireless Module**:  
  - Feasycom DB004-BT836B (Bluetooth 5.0)
- 💾 **Storage**:  
  - 64GB onboard storage for long-duration recordings

---

## 🗺️ Test Phase Roadmap

### 1️⃣ Passive & Active Noise Cancellation
- Measure baseline passive attenuation  
- Implement ANC on a **single earcup**
- Tune filters before scaling to stereo operation

### 2️⃣ Direct ATC Recording
- Implement line-in ATC audio capture  
- Validate reliable, high-quality writes to onboard storage

### 3️⃣ Bluetooth & Mobile Integration
- Enable standard Bluetooth pairing
- Implement:
  - HFP (telephony)
  - A2DP (music streaming)
- Develop mobile interface for:
  - ANC & EQ tuning
  - Wireless download of recorded ATC audio

### 4️⃣ 🚧 T.B.D.
- Next steps to be defined based on validation results and test findings

---

## 🚀 Status
> **Experimental / Proof of Concept**  
> Hardware, firmware, and DSP algorithms are under active development.