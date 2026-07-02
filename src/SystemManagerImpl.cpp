#include <Arduino.h>
#include <ctime>
#include <cstring>
#include "SystemManagerImpl.h"
#include "../include/Config.h"

bool SystemManagerImpl::initialize() {
  systemState = SystemState::INITIALIZING;

  // Create all subsystems
  contentMgr.reset(new ContentManagerImpl());
  configLoader.reset(new ConfigLoaderImpl());
  logger.reset(new PlaybackLoggerImpl());
  playbackCtrl.reset(new PlaybackControllerImpl(contentMgr.get(), configLoader.get(), logger.get()));
  audioPlayer.reset(new AudioPlayerImpl());
  buttonMgr.reset(new ButtonManagerImpl(contentMgr.get()));

  if (!initializeSubsystems()) {
    systemState = SystemState::STANDBY;
    return false;
  }

  systemState = SystemState::READY;
  return true;
}

bool SystemManagerImpl::initializeSubsystems() {
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

  if (!audioPlayer->initialize()) {
    snprintf(lastError, sizeof(lastError), "AudioPlayer init failed: %s", audioPlayer->getLastError());
    return false;
  }

  if (!buttonMgr->initialize()) {
    snprintf(lastError, sizeof(lastError), "ButtonManager init failed: %s", buttonMgr->getLastError());
    return false;
  }

  if (!playbackCtrl->initialize()) {
    snprintf(lastError, sizeof(lastError), "PlaybackController init failed: %s", playbackCtrl->getLastError());
    return false;
  }

  return true;
}

void SystemManagerImpl::update() {
  // Poll buttons
  if (buttonMgr->poll()) {
    ButtonEvent event = buttonMgr->getLastEvent();
    handleButtonEvent(event);
  }

  // Process audio playback
  if (audioPlayer->isPlaying()) {
    if (!audioPlayer->process()) {
      // Playback finished
      playbackCtrl->notifyClipFinished();
    }
  }

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
  if (systemState == SystemState::STANDBY) {
    // Try recovery if in standby
    if (contentMgr->retrySDMount()) {
      systemState = SystemState::READY;
      playbackCtrl->exitStandby();
    } else {
      return;  // Still in standby
    }
  }

  // Get playback request from controller
  PlaybackRequest request = playbackCtrl->handleButtonEvent(event.typeIndex, event.pressTimeMs);

  // Execute playback action
  switch (request.action) {
    case PlaybackAction::START_CLIP: {
      const char* filePath = contentMgr->getVariantPath(request.typeIndex, request.variantIndex);
      if (filePath && audioPlayer->playFile(filePath)) {
        // Success
      }
      break;
    }
    case PlaybackAction::STOP_CLIP:
      audioPlayer->stop();
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

AudioPlayer* SystemManagerImpl::getAudioPlayer() {
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
