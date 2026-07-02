#include <Arduino.h>
#include <SD.h>
#include <cstring>
#include "AudioPlayerImpl.h"
#include "../include/Config.h"

AudioPlayerImpl::~AudioPlayerImpl() {
  stop();
  if (decoded) delete decoded;
  if (copier) delete copier;
  if (audioKit) delete audioKit;
}

bool AudioPlayerImpl::initialize() {
  try {
    // Create AudioBoardStream with ES8388V1 codec
    audioKit = new AudioBoardStream(AudioKitEs8388V1);
    if (!audioKit) {
      snprintf(lastError, sizeof(lastError), "Failed to allocate AudioBoardStream");
      return false;
    }

    // Configure logging
    AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);

    // Get default config in TX mode (output)
    auto cfg = audioKit->defaultConfig(TX_MODE);

    // SD is handled manually via SD.h/SPI
    cfg.sd_active = false;

    // Initialize audio kit
    if (!audioKit->begin(cfg)) {
      snprintf(lastError, sizeof(lastError), "audioKit->begin() failed");
      return false;
    }

    // Enable amplifier
    pinMode(PIN_AMP_ENABLE, OUTPUT);
    digitalWrite(PIN_AMP_ENABLE, AMP_ENABLED_STATE);

    // Set initial volume
    audioKit->setVolume(currentVolume);

    // Create decoder and copier objects
    decoded = new EncodedAudioStream(audioKit, &wav);
    copier = new StreamCopy(*decoded, currentFile);

    if (!decoded || !copier) {
      snprintf(lastError, sizeof(lastError), "Failed to allocate StreamCopy/decoder");
      return false;
    }

    return true;
  } catch (const std::exception& e) {
    snprintf(lastError, sizeof(lastError), "Initialize exception: %s", e.what());
    return false;
  }
}

bool AudioPlayerImpl::playFile(const char* filePath) {
  if (!audioKit) {
    snprintf(lastError, sizeof(lastError), "AudioKit not initialized");
    return false;
  }

  stop();  // Stop any current playback

  // Open file from SD
  currentFile = SD.open(filePath);
  if (!currentFile) {
    snprintf(lastError, sizeof(lastError), "Cannot open file: %s", filePath);
    return false;
  }

  try {
    decoded->begin();
    playing = true;
    streamState.isOpen = true;
    streamState.isPlaying = true;
    streamState.currentFilePath = filePath;
    streamState.bytesRead = 0;
    streamState.totalBytes = currentFile.size();

    Serial.print("Playing: ");
    Serial.println(filePath);
    return true;
  } catch (const std::exception& e) {
    snprintf(lastError, sizeof(lastError), "playFile exception: %s", e.what());
    currentFile.close();
    playing = false;
    return false;
  }
}

void AudioPlayerImpl::stop() {
  if (playing) {
    try {
      decoded->end();
    } catch (...) {}
  }

  if (currentFile) {
    currentFile.close();
  }

  playing = false;
  streamState.isOpen = false;
  streamState.isPlaying = false;
  streamState.bytesRead = 0;
}

bool AudioPlayerImpl::isPlaying() const {
  return playing;
}

bool AudioPlayerImpl::process() {
  if (!playing || !copier) {
    return true;  // Nothing to do
  }

  try {
    // Copy one chunk of audio data from file to I2S
    if (!copier->copy()) {
      // Copy returned false = end of file or error
      Serial.println("Playback complete");
      stop();
      return false;  // Playback finished
    }
    return true;  // Still playing
  } catch (const std::exception& e) {
    snprintf(lastError, sizeof(lastError), "process exception: %s", e.what());
    stop();
    return false;
  }
}

void AudioPlayerImpl::setVolume(float vol) {
  if (vol < 0.0f) vol = 0.0f;
  if (vol > 1.0f) vol = 1.0f;

  currentVolume = vol;
  if (audioKit) {
    audioKit->setVolume(vol);
  }
}

float AudioPlayerImpl::getVolume() const {
  return currentVolume;
}

const AudioStreamState& AudioPlayerImpl::getStreamState() const {
  return streamState;
}

bool AudioPlayerImpl::isHealthy() const {
  return audioKit != nullptr;
}

const char* AudioPlayerImpl::getLastError() const {
  return lastError;
}
