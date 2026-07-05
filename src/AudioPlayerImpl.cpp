#include <Arduino.h>
#include <SD.h>
#include <cstring>
#include "AudioPlayerImpl.h"
#include "../include/Config.h"

// WAV header parser — validates before decoder touches file
struct WavHeaderInfo {
  uint32_t sampleRate = 0;
  uint16_t channels = 0;
  uint16_t bitsPerSample = 0;
};

static bool readU32LE(File& f, uint32_t& out) {
  uint8_t b[4];
  if (f.read(b, 4) != 4) return false;
  out = uint32_t(b[0]) | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24);
  return true;
}

static bool readU16LE(File& f, uint16_t& out) {
  uint8_t b[2];
  if (f.read(b, 2) != 2) return false;
  out = uint16_t(b[0]) | (uint16_t(b[1]) << 8);
  return true;
}

static bool readFourCC(File& f, char out[5]) {
  if (f.read((uint8_t*)out, 4) != 4) return false;
  out[4] = '\0';
  return true;
}

static bool parseWavHeader(File& f, WavHeaderInfo& info) {
  f.seek(0);

  char riff[5], wave[5];
  uint32_t riffSize;

  if (!readFourCC(f, riff) || strcmp(riff, "RIFF") != 0) return false;
  if (!readU32LE(f, riffSize)) return false;
  if (!readFourCC(f, wave) || strcmp(wave, "WAVE") != 0) return false;

  bool foundFmt = false, foundData = false;

  while (f.available() && !foundData) {
    char chunkId[5];
    uint32_t chunkSize;

    if (!readFourCC(f, chunkId) || !readU32LE(f, chunkSize)) return false;

    uint32_t chunkStart = f.position();

    if (strcmp(chunkId, "fmt ") == 0) {
      uint16_t audioFormat;
      uint32_t byteRate;
      uint16_t blockAlign;

      if (!readU16LE(f, audioFormat)) return false;
      if (!readU16LE(f, info.channels)) return false;
      if (!readU32LE(f, info.sampleRate)) return false;
      if (!readU32LE(f, byteRate)) return false;
      if (!readU16LE(f, blockAlign)) return false;
      if (!readU16LE(f, info.bitsPerSample)) return false;

      // PCM only (audioFormat == 1)
      if (audioFormat != 1) return false;
      foundFmt = true;
    } else if (strcmp(chunkId, "data") == 0) {
      foundData = true;
    }

    // Seek to next chunk (word-aligned)
    uint32_t nextChunk = chunkStart + chunkSize;
    if (chunkSize & 1) nextChunk++;
    f.seek(nextChunk);
  }

  if (!foundFmt || !foundData) return false;

  // Validate ESP32-safe format
  if (info.sampleRate == 0 || info.channels == 0 || info.bitsPerSample == 0) return false;
  if (info.bitsPerSample != 16) return false;
  if (info.channels > 2) return false;

  f.seek(0);
  return true;
}

AudioPlayerImpl::~AudioPlayerImpl() {
  stop();
  if (decoded) delete decoded;
  if (copier) delete copier;
  if (wav) delete wav;
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

    // Decoder/copier created per-file in playFile(), not here
    // Just verify audioKit is ready

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

  Serial.print("[AUDIO] Requested: ");
  Serial.println(filePath);

  stop();

  // Open + validate file
  currentFile = SD.open(filePath);
  if (!currentFile || currentFile.size() == 0) {
    snprintf(lastError, sizeof(lastError), "Cannot open or empty: %s", filePath);
    Serial.print("[AUDIO] ERROR: ");
    Serial.println(lastError);
    return false;
  }

  Serial.print("[AUDIO] File size: ");
  Serial.print(currentFile.size());
  Serial.println(" bytes");

  // Parse WAV header BEFORE decoder
  WavHeaderInfo wavInfo;
  if (!parseWavHeader(currentFile, wavInfo)) {
    snprintf(lastError, sizeof(lastError), "Invalid/unsupported WAV: %s", filePath);
    Serial.print("[AUDIO] ERROR: ");
    Serial.println(lastError);
    currentFile.close();
    return false;
  }

  Serial.print("[AUDIO] WAV: ");
  Serial.print(wavInfo.sampleRate);
  Serial.print("Hz ");
  Serial.print(wavInfo.channels);
  Serial.print("ch ");
  Serial.print(wavInfo.bitsPerSample);
  Serial.println("bit PCM");

  try {
    // Configure audio output BEFORE decoder chain
    audio_tools::AudioInfo info(wavInfo.sampleRate, wavInfo.channels, wavInfo.bitsPerSample);
    audioKit->setAudioInfo(info);

    // Rebuild decoder chain fresh
    if (copier) {
      delete copier;
      copier = nullptr;
    }

    if (decoded) {
      decoded->end();
      delete decoded;
      decoded = nullptr;
    }

    if (wav) {
      wav->end();
      delete wav;
      wav = nullptr;
    }

    wav = new audio_tools::WAVDecoder();
    decoded = new audio_tools::EncodedAudioStream(audioKit, wav);
    copier = new audio_tools::StreamCopy(*decoded, currentFile);

    if (!wav || !decoded || !copier) {
      snprintf(lastError, sizeof(lastError), "Failed to allocate decoder");
      currentFile.close();
      return false;
    }

    // Begin playback
    Serial.println("[AUDIO] Before decoded->begin()");
    decoded->begin();
    Serial.println("[AUDIO] After decoded->begin()");

    playing = true;
    streamState.isOpen = true;
    streamState.isPlaying = true;
    streamState.currentFilePath = filePath;
    streamState.bytesRead = 0;
    streamState.totalBytes = currentFile.size();

    Serial.print("[AUDIO] Playing: ");
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
  playing = false;  // Set first so concurrent paths know playback is stopping

  if (decoded) {
    try {
      decoded->end();
    } catch (...) {}
  }

  if (currentFile) {
    currentFile.close();
  }

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
