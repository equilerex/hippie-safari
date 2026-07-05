#include <Arduino.h>
#include <ctime>
#include <cstring>
#include <string.h>
#include "SystemManagerImpl.h"
#include "ButtonManagerImpl.h"
#include "../include/Config.h"

// Global pointer for ISR (only one SystemManager instance)
static SystemManagerImpl* g_systemManager = nullptr;

// ISR for PCF8574 INT pin (GPIO 19) - MUST be fast, no I2C calls
void IRAM_ATTR onPCF8574IntISR() {
  if (g_systemManager) {
    // Just set flag - main loop handles I2C to avoid ISR timeout
    ButtonManagerImpl* btnMgr = static_cast<ButtonManagerImpl*>(g_systemManager->getButtonManager());
    if (btnMgr) {
      btnMgr->interruptPending = true;  // Non-blocking flag
    }
  }
}

bool SystemManagerImpl::initialize() {
  systemState = SystemState::INITIALIZING;

  // Save global pointer for ISR
  g_systemManager = this;

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

  // Attach interrupt handler to PCF8574 INT pin (GPIO 19)
  Serial.println("[DEBUG] Attaching PCF8574 INT handler to GPIO 19...");
  pinMode(19, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(19), onPCF8574IntISR, FALLING);

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
  // Handle PCF8574 interrupt flag (set by ISR, handled here to avoid I2C in ISR)
  if (buttonMgr) {
    ButtonManagerImpl* btnMgr = static_cast<ButtonManagerImpl*>(buttonMgr.get());
    if (btnMgr && btnMgr->interruptPending) {
      btnMgr->interruptPending = false;
      btnMgr->onPCF8574Interrupt();  // Safe I2C calls in main loop
    }
  }

  // Dequeue button events from interrupt queue (non-blocking)
  ButtonEvent event;
  while (buttonMgr && buttonMgr->dequeueEvent(event)) {
    handleButtonEvent(event);
  }

  // Process audio playback
  bool isPlaying = audioPlayer->isPlaying();
  if (isPlaying) {
    if (!audioPlayer->process()) {
      // Playback finished
      playbackCtrl->notifyClipFinished();
      wasPlayingLastFrame = false;
      displayMgr->showStandby();
    }
  } else if (wasPlayingLastFrame) {
    // Transitioned from playing to not playing
    wasPlayingLastFrame = false;
    displayMgr->showStandby();
  }
  wasPlayingLastFrame = isPlaying;

  // Check if recovery is needed
  if (systemState == SystemState::STANDBY || systemState == SystemState::RECOVERING) {
    processRecovery();
  }

  // Track when audio stopped
  bool isNowPlaying = audioPlayer->isPlaying();
  if (wasPlayingLastFrame && !isNowPlaying) {
    audioStoppedMs = millis();
  }

  // Flush logs only during quiet window: audio stopped 5+ seconds ago + no interaction in last 5 seconds
  // This prevents SD blocking during active playback or button interaction
  uint32_t now = millis();
  uint32_t timeSinceAudioStopped = (audioStoppedMs > 0) ? (now - audioStoppedMs) : 0;
  uint32_t timeSinceInteraction = (lastInteractionMs > 0) ? (now - lastInteractionMs) : now;

  if (!isNowPlaying && logger->queueNeedsFlushing() &&
      timeSinceAudioStopped >= 5000 && timeSinceInteraction >= 5000) {
    logger->flushQueue();
  }
}

void SystemManagerImpl::handleButtonEvent(const ButtonEvent& event) {
  // Record interaction time for quiet-window flush deferral
  lastInteractionMs = millis();

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

  Serial.print("[DEBUG] Button request: type=");
  Serial.print(event.typeIndex);
  Serial.print(" variant=");
  Serial.print(request.variantIndex);
  Serial.print(" action=");
  Serial.println(request.action == PlaybackAction::START_CLIP ? "START" : "STOP");

  // Execute playback action
  switch (request.action) {
    case PlaybackAction::START_CLIP: {
      const char* filePath = contentMgr->getVariantPath(request.typeIndex, request.variantIndex);
      if (filePath) {
        // Start audio playback FIRST (non-blocking)
        if (audioPlayer->playFile(filePath)) {
          wasPlayingLastFrame = true;
          lastInteractionMs = millis();  // Update interaction timer on playback start
        }
        // Update OLED AFTER audio starts (blocking I2C doesn't delay button polling)
        // Extract basename for display
        const char* basename = strrchr(filePath, '/');
        if (basename) basename++; else basename = filePath;
        displayMgr->showNowPlaying(basename);
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
