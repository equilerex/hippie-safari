#pragma once

#include <cstdint>
#include "AudioTools.h"
#include "../include/AudioPlayer.h"

using namespace audio_tools;

class AudioPlayerImpl : public AudioPlayer {
private:
  AudioBoardStream* audioKit = nullptr;
  File currentFile;
  WAVDecoder wav;
  EncodedAudioStream* decoded = nullptr;
  StreamCopy* copier = nullptr;
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

private:
  bool isPlaying() const;
};

#endif // AUDIO_PLAYER_IMPL_H
