#pragma once

#include <memory>
#include "../include/SystemManager.h"
#include "ContentManagerImpl.h"
#include "ConfigLoaderImpl.h"
#include "PlaybackControllerImpl.h"
#include "AudioPlayerImpl.h"
#include "ButtonManagerImpl.h"
#include "PlaybackLoggerImpl.h"

class SystemManagerImpl : public SystemManager {
private:
  std::unique_ptr<ContentManagerImpl> contentMgr;
  std::unique_ptr<ConfigLoaderImpl> configLoader;
  std::unique_ptr<PlaybackLoggerImpl> logger;
  std::unique_ptr<PlaybackControllerImpl> playbackCtrl;
  std::unique_ptr<AudioPlayerImpl> audioPlayer;
  std::unique_ptr<ButtonManagerImpl> buttonMgr;

  SystemState systemState = SystemState::INITIALIZING;
  uint32_t lastRetryMs = 0;
  char lastError[256] = {0};

public:
  SystemManagerImpl() = default;
  ~SystemManagerImpl() override = default;

  bool initialize() override;
  void update() override;
  bool isReady() const override;
  SystemState getSystemState() const override;
  ContentManager* getContentManager() override;
  ConfigLoader* getConfigLoader() override;
  PlaybackController* getPlaybackController() override;
  AudioPlayer* getAudioPlayer() override;
  ButtonManager* getButtonManager() override;
  PlaybackLogger* getPlaybackLogger() override;
  const char* getLastError() const override;

private:
  bool initializeSubsystems();
  void handleButtonEvent(const ButtonEvent& event);
  void processRecovery();
};

#endif // SYSTEM_MANAGER_IMPL_H
