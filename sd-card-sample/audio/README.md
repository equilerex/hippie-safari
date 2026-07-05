# 🎵 Hippy Safari — Audio File Preparation Guide

This guide is for the sound designer/editor preparing audio content for the Hippy Safari installation. 

Please follow these specifications exactly. The ESP32 micro-controller has tight memory and CPU constraints and **will crash or fail to play** files that do not match the required format.

---

## 📋 Audio Format Specifications (CRITICAL)

All audio files—both normal tracks and easter eggs—must be exported with the following settings:

| Parameter | Requirement | Notes |
| :--- | :--- | :--- |
| **Format** | **WAV (PCM)** | Must be uncompressed PCM WAV. (Do not use MP3, AAC, or compressed WAV). |
| **Bit Depth** | **16-bit** | Do not use 24-bit or 32-bit (they will fail to play). |
| **Sample Rate** | **22050 Hz** | Must be exactly **22050 Hz**. Lower rates will play too fast; higher rates (like 44100 Hz or 48000 Hz) will stutter or overflow the ESP32's buffer. |
| **Channels** | **Mono** (recommended) or **Stereo** | Mono is highly recommended to save SD card bandwidth and ESP32 RAM. |

---

## 🛠️ How to Export / Convert Files

### Using Audacity (Free)
1. Open your audio file in Audacity.
2. In the bottom-left corner, set the **Project Rate (Hz)** to `22050`.
3. If the file is stereo, you can optionally convert to mono by going to **Tracks > Mix > Mix Stereo Down to Mono** (saves space).
4. Go to **File > Export > Export as WAV**.
5. Set **Encoding** to `Signed 16-bit PCM`.
6. Save the file directly to the correct SD card folder.

### Using FFmpeg (Command Line)
To convert any file to the correct format:
```bash
ffmpeg -i input.mp3 -acodec pcm_s16le -ac 1 -ar 22050 output.wav
```

---

## 📁 SD Card Folder Structure

The SD card must have two main directories under `/audio/`:
1. `/audio/types/` (for normal button playbacks)
2. `/audio/easter-egg/` (for hidden pattern playbacks)

---

## 🟢 1. Normal Content (`/audio/types/`)

Normal playbacks are triggered when visitors press one of the physical installation buttons.

### Directory Mapping
Each button corresponds to a numbered folder under `/audio/types/` (ordered alphabetically by folder name):
* Button 1 maps to the first folder (e.g., `01_intro/`)
* Button 2 maps to the second folder (e.g., `02_plain-clothes/`)
* Button 3 maps to the third folder (e.g., `03_groups/`)

### File Cycling (Variants)
Within each type folder, place one or more `.wav` files (e.g., `track1.wav`, `track2.wav`, `track3.wav`).
* When a visitor presses Button 1, it plays the first file.
* If they press Button 1 again, it cycles to the next file alphabetically.
* Once it reaches the last file, it wraps back to the first.

---

## 🟡 2. Easter Eggs (`/audio/easter-egg/`)

Easter eggs are hidden audio clips triggered by specific interaction patterns on the buttons.

### Creating Easter Eggs
To enable an easter egg, create a folder under `/audio/easter-egg/` named exactly after the pattern (see table below), and place your `.wav` files inside it.

| Folder Name | Trigger Pattern | Suggested Content |
| :--- | :--- | :--- |
| **`SECRET_BUTTON`** | Pressing the hidden developer/secret button (P7). | "You found the hidden frequency..." |
| **`ASCENDING_SWEEP`** | Pressing buttons in order: Button 1 → Button 2 → Button 3 within 2 seconds. | Ascending narration or rising sound effect. |
| **`SOS_MORSE`** | Rapidly clicking buttons ~9 times in 5 seconds (simulating SOS). | Distress signals, radio static, or animal alerts. |
| **`HAMMER_SINGLE`** | Rapidly mashing the *same* button 15+ times in 3 seconds. | Woodpecker sound effects or sarcastic commentary about impatience. |
| **`TEAM_EFFORT`** | Pressing all buttons simultaneously (within 100ms). | Chorus of animal calls or synchronized narration. |
| **`LONG_HOLD_SUSTAINED`**| Pressing and holding any button down for 5+ seconds. | Breathing sounds, ambient drones, or commentary on patience. |
| **`MULTI_HOLD`** | Pressing and holding 2+ buttons together for 3+ seconds. | Harmonic duets, overlapping vocal tracks, or coordination sounds. |
| **`ALL_BUTTONS_HELD`** | Pressing and holding all buttons together for 5+ seconds. | Orchestral crescendo or climax audio. |
| **`CHAOS_BURST`** | Rapidly mashing random buttons 12+ times in 4 seconds. | Animal stampede or chaotic sounds. |
| **`MULTI_CLICK`** | Alternately clicking 2+ buttons 4+ times total in 2 seconds. | Conversational call-and-response between different species. |

### Variant Looping
If an easter egg folder contains multiple files (e.g., `01.wav`, `02.wav`), the system cycles through them on successive triggers. The cycle resets back to the first file after **5 minutes of silence**.

---

## 🔵 3. Testing Playback Modes (Debugging)

To verify content layouts, mode switches, and variant cycles in real-time without waiting for specific hours of the day, you can configure a test window inside your mode's `config.json` file:

```json
{
  "priority": 999,
  "timeWindows": [
    {
      "days": ["test"],
      "startTime": "00:00",
      "endTime": "23:59",
      "priority": 999
    }
  ]
}
```

Setting `"days": ["test"]` (or `"test_odd"`) forces the time window to match **only during odd/uneven minutes** of the RTC clock (e.g., 14:01, 14:03). This automatically switches the system back to `"default"` mode during even minutes (e.g., 14:02, 14:04) and triggers the custom mode during odd minutes, letting you verify folder transitions simply by waiting for the calendar minute to change.
