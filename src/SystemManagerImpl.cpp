#include <Arduino.h>
#include <ctime>
#include <cstring>
#include <string.h>
#include "SystemManagerImpl.h"
#include "../include/Config.h"

bool SystemManagerImpl::initialize() {
  systemState = SystemState::INITIALIZING;

  // Create I2C access mutex (serialize audio codec + external I2C)
  i2cMutex = xSemaphoreCreateMutex();
  if (!i2cMutex) {
    Serial.println("[ERROR] Failed to create I2C mutex");
    return false;
  }

  // Create all subsystems (except PCF8574 — needs extI2C bus initialized first)
  contentMgr.reset(new ContentManagerImpl());
  configLoader.reset(new ConfigLoaderImpl());
  logger.reset(new PlaybackLoggerImpl());
  playbackCtrl.reset(new PlaybackControllerImpl(contentMgr.get(), configLoader.get(), logger.get()));
  audioPlayer.reset(new AudioPlayerImpl());
  rtcMgr.reset(new RtcManagerImpl(logger.get()));
  displayMgr.reset(new DisplayManagerImpl());

  if (!initializeSubsystems()) {
    systemState = SystemState::STANDBY;
    return false;
  }

  systemState = SystemState::READY;
  return true;
}

bool SystemManagerImpl::initializeSubsystems() {
  // Initialize external I2C bus (shared by RTC, OLED, PCF8574)
  Serial.print("[DEBUG] Initializing external I2C bus: SDA=GPIO");
  Serial.print(PIN_EXT_I2C_SDA);
  Serial.print(", SCL=GPIO");
  Serial.println(PIN_EXT_I2C_SCL);

  int result = extI2C.begin(PIN_EXT_I2C_SDA, PIN_EXT_I2C_SCL);
  Serial.print("[DEBUG] extI2C.begin() returned: ");
  Serial.println(result);
  Serial.println("[DEBUG] External I2C bus ready");

  // Debug: try to ping devices on extI2C
  Serial.println("[DEBUG] Scanning extI2C for devices...");
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    extI2C.beginTransmission(addr);
    int error = extI2C.endTransmission();
    if (error == 0) {
      Serial.print("[DEBUG] Found I2C device at 0x");
      if (addr < 0x10) Serial.print("0");
      Serial.println(addr, HEX);
    }
  }

  // Construct PCF8574 after bus is ready, passing extI2C explicitly
  Serial.println("[DEBUG] Constructing PCF8574 on extI2C...");
  pcf8574.reset(new PCF8574(0x20, &extI2C));  // PCF8574 at I2C addr 0x20 on extI2C bus
  buttonMgr.reset(new ButtonManagerImpl(contentMgr.get(), pcf8574.get()));

  // Initialize in dependency order
  if (!contentMgr->initialize()) {
    snprintf(lastError, sizeof(lastError), "ContentManager init failed: %s", contentMgr->getLastError());
    systemState = SystemState::STANDBY;
    return false;
  }

  if (!logger->initialize()) {
    snprintf(lastError, sizeof(lastError), "PlaybackLogger init failed: %s", logger->getLastError());
    // Logger failure is not fatal
  }

  // Initialize RTC (non-fatal; system works without it)
  Serial.println("[DEBUG] Initializing RTC...");
  if (!rtcMgr->initialize(&extI2C)) {
    Serial.print("[DEBUG] RTC init warning: ");
    Serial.println(rtcMgr->getLastError());
    // Non-fatal; mode selection will fall back to "default" mode
  } else {
    Serial.println("[DEBUG] RTC initialized successfully");
  }

  // Initialize OLED display (non-fatal; system works without it)
  displayMgr->setI2CMutex(i2cMutex);  // Pass mutex for I2C serialization
  Serial.println("[DEBUG] Initializing OLED display...");
  if (!displayMgr->initialize(&extI2C)) {
    Serial.print("[DEBUG] OLED init warning: ");
    Serial.println(displayMgr->getLastError());
    // Non-fatal
  } else {
    Serial.println("[DEBUG] OLED initialized successfully");
  }

  if (!audioPlayer->initialize()) {
    snprintf(lastError, sizeof(lastError), "AudioPlayer init failed: %s", audioPlayer->getLastError());
    return false;
  }

  Serial.println("[DEBUG] Initializing ButtonManager...");
  if (!buttonMgr->initialize()) {
    Serial.print("[DEBUG] ButtonManager init warning: ");
    Serial.println(buttonMgr->getLastError());
    Serial.println("[WARNING] Buttons disabled — PCF8574 not responding. Check I2C wiring.");
    // Non-fatal; system continues but buttons won't work
  } else {
    Serial.println("[DEBUG] ButtonManager initialized successfully");
  }

  if (!playbackCtrl->initialize()) {
    snprintf(lastError, sizeof(lastError), "PlaybackController init failed: %s", playbackCtrl->getLastError());
    return false;
  }

  return true;
}

void SystemManagerImpl::update() {
  // Poll buttons (only if ButtonManager initialized successfully)
  if (buttonMgr && buttonMgr->poll()) {
    ButtonEvent event = buttonMgr->getLastEvent();
    handleButtonEvent(event);
  }

  // Process audio playback
  bool isPlaying = audioPlayer->isPlaying();
  if (isPlaying) {
    if (!audioPlayer->process()) {
      // Playback finished
      playbackCtrl->notifyClipFinished();
      wasPlayingLastFrame = false;
      displayMgr->showStandby();  // Update OLED only on state change (playback -> standby)
    }
  } else if (wasPlayingLastFrame) {
    // Transitioned from playing to not playing; update OLED if not already done
    wasPlayingLastFrame = false;
    displayMgr->showStandby();
  }
  wasPlayingLastFrame = isPlaying;

  // Check if recovery is needed
  if (systemState == SystemState::STANDBY || systemState == SystemState::RECOVERING) {
    processRecovery();
  }

  // Flush logs if queue has events and playback is idle
  if (!audioPlayer->isPlaying() && logger->queueNeedsFlushing()) {
    logger->flushQueue();
  }
}

void SystemManagerImpl::handleButtonEvent(const ButtonEvent& event) {
  // Validate event: typeIndex must be in bounds
  if (event.typeIndex >= contentMgr->getTypeCount()) {
    Serial.print("[WARNING] Invalid button event: typeIndex ");
    Serial.print(event.typeIndex);
    Serial.print(" >= typeCount ");
    Serial.println(contentMgr->getTypeCount());
    return;
  }

  Serial.print("[DEBUG] Button event received: typeIndex=");
  Serial.println(event.typeIndex);

  if (systemState == SystemState::STANDBY) {
    // Try recovery if in standby
    if (contentMgr->retrySDMount()) {
      systemState = SystemState::READY;
      playbackCtrl->exitStandby();
      displayMgr->showStandby();
    } else {
      displayMgr->showDebug("SD Card Error", "Recovery...");
      return;  // Still in standby
    }
  }

  // Get current time for mode selection (RTC if available, else 0 = fallback to default mode)
  time_t currentTime = rtcMgr->now();

  // Get playback request from controller (using real time for mode selection, millis for debug timestamps)
  PlaybackRequest request = playbackCtrl->handleButtonEvent(event.typeIndex, currentTime);

  // Execute playback action
  switch (request.action) {
    case PlaybackAction::START_CLIP: {
      const char* filePath = contentMgr->getVariantPath(request.typeIndex, request.variantIndex);
      if (filePath && audioPlayer->playFile(filePath)) {
        // Extract basename for display (called once on start, not every loop)
        const char* basename = strrchr(filePath, '/');
        if (basename) basename++; else basename = filePath;
        displayMgr->showNowPlaying(basename);
        wasPlayingLastFrame = true;
      }
      break;
    }
    case PlaybackAction::STOP_CLIP:
      audioPlayer->stop();
      displayMgr->showStandby();
      break;
    default:
      break;
  }
}

void SystemManagerImpl::processRecovery() {
  uint32_t now = millis();

  if (now - lastRetryMs > SD_RETRY_INTERVAL_MS) {
    lastRetryMs = now;

    // Try to remount SD
    if (contentMgr->retrySDMount()) {
      // SD recovered - attempt full initialization
      if (initializeSubsystems()) {
        systemState = SystemState::READY;
        playbackCtrl->exitStandby();
        logger->logSDRecovered();
        Serial.println("System recovered from standby");
      }
    }
  }
}

bool SystemManagerImpl::isReady() const {
  return systemState == SystemState::READY;
}

SystemState SystemManagerImpl::getSystemState() const {
  return systemState;
}

ContentManager* SystemManagerImpl::getContentManager() {
  return contentMgr.get();
}

ConfigLoader* SystemManagerImpl::getConfigLoader() {
  return configLoader.get();
}

PlaybackController* SystemManagerImpl::getPlaybackController() {
  return playbackCtrl.get();
}

IAudioPlayer* SystemManagerImpl::getAudioPlayer() {
  return audioPlayer.get();
}

ButtonManager* SystemManagerImpl::getButtonManager() {
  return buttonMgr.get();
}

PlaybackLogger* SystemManagerImpl::getPlaybackLogger() {
  return logger.get();
}

const char* SystemManagerImpl::getLastError() const {
  return lastError;
}
