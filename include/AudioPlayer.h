#pragma once

#include <cstdint>
#include "Config.h"
#include "ContentTypes.h"

class AudioPlayer {
public:
  virtual ~AudioPlayer() = default;

  // Initialize I2S, codec, and amplifier
  virtual bool initialize() = 0;

  // Start playing a WAV file from SD
  virtual bool playFile(const char* filePath) = 0;

  // Stop current playback
  virtual void stop() = 0;

  // Check if currently playing
  virtual bool isPlaying() const = 0;

  // Process playback: copy audio data from file to I2S output
  // Call in main loop every cycle
  virtual bool process() = 0;

  // Set volume (0.0 to 1.0)
  virtual void setVolume(float vol) = 0;

  // Get current volume
  virtual float getVolume() const = 0;

  // Get current stream state
  virtual const AudioStreamState& getStreamState() const = 0;

  // Check if codec is healthy (can add diagnostics later)
  virtual bool isHealthy() const = 0;

  // Get last playback error
  virtual const char* getLastError() const = 0;
};

#endif // AUDIO_PLAYER_H
