#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "AudioTools.h"
#include "AudioTools/AudioLibs/AudioBoardStream.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"

using namespace audio_tools;

// AudioKit v2.2 / ES8388 V1 profile.
// Your test proved this is the working profile.
AudioBoardStream kit(AudioKitEs8388V1);

// SD card pins already proven working on your board.
#define SD_CS   13
#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCK  14

// Built-in button pins commonly used on ESP32-A1S AudioKit v2.2.
// Active LOW: pressed = LOW.
#define BTN_1 5
#define BTN_2 18
#define BTN_3 19
#define BTN_4 23

// Speaker amp enable.
// Your sine test worked with this polarity; keep it the same.
// If your working sine test used HIGH, change LOW to HIGH here.
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

  // Critical: SD is handled manually via SD.h/SPI.
  // Do not let the AudioKit profile initialize SD too.
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

      // Rising/falling edge update.
      bool wasHigh = button.lastState == HIGH;
      bool isLow = state == LOW;

      button.lastState = state;

      // Button pressed: HIGH -> LOW
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