#ifndef SYSTEMMANAGERIMPL_H
#define SYSTEMMANAGERIMPL_H

#include <memory>
#include <Wire.h>
#include <PCF8574.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../include/SystemManager.h"
#include "ContentManagerImpl.h"
#include "ConfigLoaderImpl.h"
#include "PlaybackControllerImpl.h"
#include "AudioPlayerImpl.h"
#include "ButtonManagerImpl.h"
#include "PlaybackLoggerImpl.h"
#include "RtcManagerImpl.h"
#include "DisplayManagerImpl.h"

class SystemManagerImpl : public SystemManager {
private:
  // External I2C bus (RTC + PCF8574 buttons on GPIO18/23)
  TwoWire extI2C{1};
  // Note: OLED uses SW_I2C (bit-banging) on GPIO5/22, doesn't need TwoWire object
  std::unique_ptr<PCF8574> pcf8574;

  std::unique_ptr<ContentManagerImpl> contentMgr;
  std::unique_ptr<ConfigLoaderImpl> configLoader;
  std::unique_ptr<PlaybackLoggerImpl> logger;
  std::unique_ptr<RtcManagerImpl> rtcMgr;
  std::unique_ptr<DisplayManagerImpl> displayMgr;
  std::unique_ptr<PlaybackControllerImpl> playbackCtrl;
  std::unique_ptr<AudioPlayerImpl> audioPlayer;
  std::unique_ptr<ButtonManagerImpl> buttonMgr;

  SystemState systemState = SystemState::INITIALIZING;
  uint32_t lastRetryMs = 0;
  uint32_t lastInteractionMs = 0;  // Track last button press for quiet-window flush
  uint32_t audioStoppedMs = 0;  // Track when audio stopped
  char lastError[256] = {0};
  bool wasPlayingLastFrame = false;  // Track playback state for OLED updates on finish
  SemaphoreHandle_t i2cMutex = nullptr;  // Serialize I2C access (extI2C + codec I2C)

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
  IAudioPlayer* getAudioPlayer() override;
  ButtonManager* getButtonManager() override;
  PlaybackLogger* getPlaybackLogger() override;
  const char* getLastError() const override;

private:
  bool initializeSubsystems();
  void handleButtonEvent(const ButtonEvent& event);
  void processRecovery();
};

#endif // SYSTEMMANAGERIMPL_H
