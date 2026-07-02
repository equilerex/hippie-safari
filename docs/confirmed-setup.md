Yes. Here’s the slim version with the archeology scraped off.

# ESP32-A1S AudioKit v2.2 — Confirmed Setup

## Goal

Build a soundboard app:

```text
Built-in button press
→ play WAV clip from SD card
→ output through onboard ES8388 audio codec
```

Confirmed working:

```text
PlatformIO build/upload
Serial monitor
SD card read over SPI
ES8388 audio output
Built-in button-triggered WAV playback
```

---

## Environment

```text
IDE:        CLion
Build:      PlatformIO
Framework:  Arduino
Platform:   Espressif 32
Board:      esp32dev
Env name:   esp32-a1s-audiokit
OS:         Windows 11
```

Use the env name in commands:

```powershell
pio run -e esp32-a1s-audiokit
pio run -e esp32-a1s-audiokit -t upload
pio device monitor -e esp32-a1s-audiokit
```

---

## `platformio.ini`

```ini
[env:esp32-a1s-audiokit]
platform = espressif32
board = esp32dev
framework = arduino

monitor_speed = 115200
upload_speed = 115200

board_build.partitions = huge_app.csv
board_build.flash_mode = dio

build_flags =
  -DCORE_DEBUG_LEVEL=1
  -DBOARD_HAS_PSRAM
  -mfix-esp32-psram-cache-issue

lib_deps =
  https://github.com/pschatzmann/arduino-audio-tools.git#v1.1.3
  https://github.com/pschatzmann/arduino-audio-driver.git#v0.1.4
```

---

## Confirmed Board Details

```text
Board:       AI-Thinker ESP32-A1S AudioKit v2.2
Codec:      ES8388
Codec addr: 0x10
```

### AudioTools board profile

Use:

```cpp
AudioBoardStream kit(AudioKitEs8388V1);
```

Do not use `AudioKitEs8388V2`.

Reason:

```text
AudioKitEs8388V1 → I2C SCL=32, SDA=33
AudioKitEs8388V2 → I2C SCL=23, SDA=18
```

Confirmed board wiring:

```text
I2C SCL: GPIO32
I2C SDA: GPIO33
I2S MCLK: GPIO0
I2S BCLK: GPIO27
I2S WS:   GPIO25
I2S DOUT: GPIO26
I2S DIN:  GPIO35
```

---

## SD Card

Use FAT32.

Confirmed SPI pins:

```cpp
#define SD_CS   13
#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCK  14
```

Recommended SD layout:

```text
/sfx/btn1.wav
/sfx/btn2.wav
/sfx/btn3.wav
/sfx/btn4.wav
```

Recommended WAV format:

```text
PCM WAV
16-bit
44100 Hz
stereo for first tests
```

---

## Built-in Buttons

Confirmed button pins:

```cpp
#define BTN_1 5
#define BTN_2 18
#define BTN_3 19
#define BTN_4 23
```

Buttons are active-low:

```text
Released = HIGH
Pressed  = LOW
```

Use:

```cpp
pinMode(pin, INPUT_PULLUP);
```

---

## Amp Enable

Confirmed amp pin:

```cpp
#define AMP_ENABLE 21
```

Confirmed working state:

```cpp
#define AMP_ENABLED_STATE LOW
```

---

## Critical Audio Config

When using `AudioBoardStream`, disable AudioKit’s internal SD setup:

```cpp
auto cfg = kit.defaultConfig(TX_MODE);
cfg.sd_active = false;
kit.begin(cfg);
```

Reason: SD is handled manually through `SD.h` over the confirmed SPI pins.

---

## Working Button WAV Test

```cpp
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "AudioTools.h"
#include "AudioTools/AudioLibs/AudioBoardStream.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"

using namespace audio_tools;

AudioBoardStream kit(AudioKitEs8388V1);

#define SD_CS   13
#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCK  14

#define BTN_1 5
#define BTN_2 18
#define BTN_3 19
#define BTN_4 23

#define AMP_ENABLE 21
#define AMP_ENABLED_STATE LOW

File audioFile;
WAVDecoder wav;
EncodedAudioStream decoded(&kit, &wav);
StreamCopy copier(decoded, audioFile);

bool playing = false;

struct Button {
  uint8_t pin;
  const char* file;
  bool lastState;
  uint32_t lastChangeMs;
};

Button buttons[] = {
  { BTN_1, "/sfx/btn1.wav", HIGH, 0 },
  { BTN_2, "/sfx/btn1.wav", HIGH, 0 },
  { BTN_3, "/sfx/btn1.wav", HIGH, 0 },
  { BTN_4, "/sfx/btn1.wav", HIGH, 0 },
};

void stopClip() {
  if (playing) {
    decoded.end();
    playing = false;
  }

  if (audioFile) {
    audioFile.close();
  }
}

void playClip(const char* path) {
  Serial.print("Play request: ");
  Serial.println(path);

  stopClip();

  audioFile = SD.open(path);
  if (!audioFile) {
    Serial.print("Could not open ");
    Serial.println(path);
    return;
  }

  decoded.begin();
  playing = true;

  Serial.print("Playing ");
  Serial.println(path);
}

void setupSD() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, 1000000)) {
    Serial.println("SD init failed");
    while (true) delay(1000);
  }

  Serial.println("SD ready");
}

void setupAudio() {
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);

  auto cfg = kit.defaultConfig(TX_MODE);
  cfg.sd_active = false;

  if (!kit.begin(cfg)) {
    Serial.println("kit.begin() failed");
    while (true) delay(1000);
  }

  pinMode(AMP_ENABLE, OUTPUT);
  digitalWrite(AMP_ENABLE, AMP_ENABLED_STATE);

  kit.setVolume(0.8);

  Serial.println("Audio ready");
}

void setupButtons() {
  for (auto& button : buttons) {
    pinMode(button.pin, INPUT_PULLUP);
    button.lastState = digitalRead(button.pin);
    button.lastChangeMs = millis();
  }

  Serial.println("Buttons ready");
}

void pollButtons() {
  const uint32_t now = millis();
  const uint32_t debounceMs = 50;

  for (auto& button : buttons) {
    bool state = digitalRead(button.pin);

    if (state != button.lastState && now - button.lastChangeMs > debounceMs) {
      button.lastChangeMs = now;

      bool wasHigh = button.lastState == HIGH;
      bool isLow = state == LOW;

      button.lastState = state;

      if (wasHigh && isLow) {
        Serial.print("Button pressed on GPIO ");
        Serial.println(button.pin);
        playClip(button.file);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32-A1S AudioKit button WAV test");

  setupSD();
  setupAudio();
  setupButtons();

  Serial.println("Ready. Press a built-in button.");
}

void loop() {
  pollButtons();

  if (playing) {
    if (!audioFile || !copier.copy()) {
      Serial.println("Clip done");
      stopClip();
    }
  }
}
```

---

## App Baseline

Recommended first production behavior:

```text
One clip at a time
New button press interrupts current clip
WAV files only initially
Button → file path mapping in code or config
```

Initial mapping:

```cpp
Button buttons[] = {
  { BTN_1, "/sfx/btn1.wav", HIGH, 0 },
  { BTN_2, "/sfx/btn2.wav", HIGH, 0 },
  { BTN_3, "/sfx/btn3.wav", HIGH, 0 },
  { BTN_4, "/sfx/btn4.wav", HIGH, 0 },
};
```

Suggested later structure:

```text
src/
  main.cpp
  SoundBoard.h
  SoundBoard.cpp
  ButtonInput.h
  ButtonInput.cpp
  ContentIndex.h
  ContentIndex.cpp
```

Do not re-check board pins unless the physical board revision changes.

That’s the version I’d feed Claude. It keeps the confirmed substrate and removes the yak-shavings.
