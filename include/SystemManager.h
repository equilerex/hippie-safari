#pragma once

#include <memory>
#include "Config.h"
#include "ContentManager.h"
#include "ConfigLoader.h"
#include "PlaybackController.h"
#include "AudioPlayer.h"
#include "ButtonManager.h"
#include "PlaybackLogger.h"

// Central orchestrator for all subsystems
class SystemManager {
public:
  SystemManager() = default;
  virtual ~SystemManager() = default;

  // Initialize all subsystems in order
  virtual bool initialize() = 0;

  // Main loop update - call every cycle
  virtual void update() = 0;

  // Check system readiness
  virtual bool isReady() const = 0;

  // Get system state
  virtual SystemState getSystemState() const = 0;

  // Component accessors (for testing/debugging)
  virtual ContentManager* getContentManager() = 0;
  virtual ConfigLoader* getConfigLoader() = 0;
  virtual PlaybackController* getPlaybackController() = 0;
  virtual IAudioPlayer* getAudioPlayer() = 0;
  virtual ButtonManager* getButtonManager() = 0;
  virtual PlaybackLogger* getPlaybackLogger() = 0;

  // Get overall system error
  virtual const char* getLastError() const = 0;
};

#endif // SYSTEM_MANAGER_H
