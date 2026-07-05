#ifndef AUDIOPLAYERIMPL_H
#define AUDIOPLAYERIMPL_H


#include <cstdint>
#include <SD.h>
#include "AudioTools.h"
#include "AudioTools/AudioLibs/AudioBoardStream.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"
#include "../include/AudioPlayer.h"

using namespace audio_tools;

class AudioPlayerImpl : public IAudioPlayer {
private:
  audio_tools::AudioBoardStream* audioKit = nullptr;
  File currentFile;
  audio_tools::WAVDecoder* wav = nullptr;  // Fresh decoder per file
  audio_tools::EncodedAudioStream* decoded = nullptr;
  audio_tools::StreamCopy* copier = nullptr;
  bool playing = false;
  float currentVolume = 0.8f;
  char lastError[256] = {0};
  AudioStreamState streamState;

public:
  AudioPlayerImpl() = default;
  ~AudioPlayerImpl() override;

  bool initialize() override;
  bool playFile(const char* filePath) override;
  void stop() override;
  bool isPlaying() const override;
  bool process() override;
  void setVolume(float vol) override;
  float getVolume() const override;
  const AudioStreamState& getStreamState() const override;
  bool isHealthy() const override;
  const char* getLastError() const override;
};


#endif // AUDIOPLAYERIMPL_H
