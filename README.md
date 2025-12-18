# AERIS
**General Aviation Headset PoC**

<div align="center">
<img width="911" height="820" alt="OP-AERIS" src="https://github.com/user-attachments/assets/33ea24da-07fe-4eed-9816-3114d598dea0" />
</div>

## Overview
AERIS is a Proof-of-Concept (PoC) project developing a high-fidelity General Aviation (GA) headset. This project integrates advanced audio features, including Active Noise Cancellation (ANC), wireless connectivity, and specialized aviation recording capabilities, aiming to deliver a premium cockpit experience.

## Key Features
- **Active Noise Cancellation (ANC)**: Advanced hybrid ANC system designed to attenuate cockpit noise.
- **Bluetooth Connectivity**:
  - **Music Streaming**: High-quality wireless audio.
  - **Telephony**: Hands-free phone call support.
  - **OTA Updates**: Over-the-air firmware update capability.
  - **ATC Downloads**: Wireless transfer of recorded Air Traffic Control audio.
- **ATC Recording**: Integrated system to record and store ATC communications.

## Hardware Specifications
AERIS is built using high-end audio components to ensure superior sound quality and effective noise reduction.

### Audio Drivers
- **Model**: AH-D9200
- **Type**: High-Fidelity Drivers
- **Description**: Sourced from premium audiophile headphones to provide exceptional clarity and dynamic range for both communications and media.

### Microphone Configuration
- **ANC Microphones**:
  - **Sensor**: ICS-43434 (MEMS)
  - **Configuration**: 4x Total (2 per earcup)
  - **Placement**: 1x External (Feedforward), 1x Internal (Feedback) per cup for hybrid noise cancellation.
- **Communications Microphone**:
  - **Model**: PA-9EHN
  - **Type**: Analog Electret
  - **Application**: Noise-canceling pilot microphone designed for clarity in noisy aviation environments.


## Test Phase Setup
To validate the AERIS concept and tune the audio processing algorithms, a test phase is currently underway. This phase utilizes a custom-modified hardware platform designed to simulate the final acoustic environment and test the electronic components in a controlled setting.

**Platform:**
- **Chassis**: Modified **3M™ PELTOR™ Optime™ III** Earmuffs. Chosen for their high passive noise attenuation, providing an ideal baseline for testing ANC efficacy.

**Electronics & Audio Components:**
- **Core Processing Unit**: **Teensy 4.1**. A powerful microcontroller capable of low-latency audio processing.
- **Audio Interface**: **Teensy Audio Adaptor Board**. Provides the necessary CODEC and I/O interfaces for the Teensy platform.
- **Speaker**: **4070 Speaker (4Ω, 3W)**. Selected to drive high-fidelity audio output within the earcup.
- **Microphones**: **2x INMP441 MEMS Microphones (I2S)**. High-performance, omnidirectional digital microphones used for capturing environmental noise and internal audio for the feedback/feedforward ANC loops.
- **Connectivity**: **Feasycom DB004-BT836B Bluetooth 5.0 Module**. Enables wireless audio streaming and data connectivity for the headset.
- **Storage**: **64GB**. Included for extensive audio recording and data storage.

### Test Phase Roadmap
The following milestones define the immediate focus of the test phase:

1. **Passive & Active Noise Cancellation Validation**:
   - Establish baseline passive attenuation.
   - Implement and tune the ANC algorithm for a single earcup to verify noise reduction performance before scaling to stereo.

2. **Direct ATC Recording**:
   - Implement the audio pipeline to capture line-in audio (simulated ATC communications).
   - Validate direct writing of high-fidelity audio data to the onboard 64GB storage.

3. **Bluetooth Integration & Mobile Connectivity**:
   - Enable the Feasycom module for standard phone pairing and HFP (Hands-Free Profile) support.
   - Implement A2DP for high-quality audio streaming.
   - Develop the mobile app interface to allow parameter tuning (ANC filters, EQ) and wireless downloading of stored ATC recordings.

4. **T.B.D.**
   - Future steps to be defined based on initial validation results.

