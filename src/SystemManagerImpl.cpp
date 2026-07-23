#include <Arduino.h>
#include <ctime>
#include <cstring>
#include <string.h>
#include "SystemManagerImpl.h"
#include "ButtonManagerImpl.h"
#include "../include/Config.h"

// Instantiate extern variables from Config.h
uint8_t NUM_BUTTON_TYPES = MAX_BUTTON_TYPES;  // Full range until content discovery narrows it below
uint8_t BUTTON_CHIP_INDEX[MAX_BUTTON_TYPES];
uint8_t BUTTON_PORT_INDEX[MAX_BUTTON_TYPES];

// Builds the identity chip/port mapping once at static-init time: every port
// across all 3 PCF8574 chips, in chip-then-port order, skipping the one port
// physically reserved for the secret button (EASTER_EGG_CHIP/EASTER_EGG_PORT).
static bool buildButtonPinMap(uint8_t (&chipOut)[MAX_BUTTON_TYPES], uint8_t (&portOut)[MAX_BUTTON_TYPES]) {
  uint8_t idx = 0;
  for (uint8_t c = 0; c < NUM_PCF8574_CHIPS && idx < MAX_BUTTON_TYPES; c++) {
    for (uint8_t p = 0; p < PCF8574_PORTS && idx < MAX_BUTTON_TYPES; p++) {
      if (c == EASTER_EGG_CHIP && p == EASTER_EGG_PORT) continue;
      chipOut[idx] = c;
      portOut[idx] = p;
      idx++;
    }
  }
  return true;
}
static bool g_buttonPinMapBuilt = buildButtonPinMap(BUTTON_CHIP_INDEX, BUTTON_PORT_INDEX);

// I2C Scanner Helper for systematic debugging (and stabilization delay)
static void scanI2CBus(TwoWire& wire, const char* label) {
  Serial.print("[I2C SCAN] ");
  Serial.print(label);
  Serial.println(":");
  int nDevices = 0;
  for (uint8_t address = 1; address < 127; address++) {
    wire.beginTransmission(address);
    byte error = wire.endTransmission();
    if (error == 0) {
      Serial.print("  -> Found device at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    } else if (error == 4) {
      Serial.print("  -> Unknown error at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("  -> No I2C devices responded");
  }
}

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

  // Start WiFi AP + log-viewer webserver first, independent of SD/audio state
  webServerMgr.reset(new WebServerManagerImpl());
  if (!webServerMgr->initialize()) {
    Serial.print("[WARNING] WebServerManager init failed: ");
    Serial.println(webServerMgr->getLastError());
    // Non-fatal; system continues without the log viewer
  }

  // Create all subsystems (except PCF8574 — needs extI2C bus initialized first)
  contentMgr.reset(new ContentManagerImpl());
  configLoader.reset(new ConfigLoaderImpl(contentMgr.get()));
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
  delay(50);  // Allow I2C pins to electrically pull high and stabilize
  Serial.print("[DEBUG] extI2C.begin() returned: ");
  Serial.println(result);
  Serial.println("[DEBUG] External I2C bus ready");
  scanI2CBus(extI2C, "At startup");

  // Skip I2C scan - it corrupts PCF8574 state before button manager init
  // TODO: Move scan to after button init if debug output needed

  // Construct all PCF8574 chips after bus is ready, passing extI2C explicitly
  Serial.println("[DEBUG] Constructing PCF8574 chips on extI2C...");
  PCF8574* chipPtrs[NUM_PCF8574_CHIPS];
  for (uint8_t c = 0; c < NUM_PCF8574_CHIPS; c++) {
    pcf8574[c].reset(new PCF8574(PCF8574_ADDRESSES[c], &extI2C));
    chipPtrs[c] = pcf8574[c].get();
  }
  buttonMgr.reset(new ButtonManagerImpl(contentMgr.get(), chipPtrs, &extI2C));

  // Attach interrupt handler to PCF8574 INT pin (GPIO 19)
  Serial.println("[DEBUG] Attaching PCF8574 INT handler to GPIO 19...");
  pinMode(19, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(19), onPCF8574IntISR, FALLING);

  // Initialize in dependency order
  // Content discovery (normal + easter eggs happen together)
  if (!contentMgr->initialize()) {
    snprintf(lastError, sizeof(lastError), "ContentManager init failed: %s", contentMgr->getLastError());
    systemState = SystemState::STANDBY;
    return false;
  }

  // Optional /safari-conf.json override for button/easter-egg timings (SD already mounted above)
  g_tuning.loadFromSD();

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

  scanI2CBus(extI2C, "After RTC init");

  // CRITICAL WARNING: RTClib begin() resets the TwoWire port 1 (extI2C) pins back to defaults.
  // We must re-establish GPIO 18/23 here. DO NOT REMOVE THIS CALL or the delay(50)!
  // The delay is electrically required to let the open-drain SDA/SCL lines pull high and stabilize.
  extI2C.begin(PIN_EXT_I2C_SDA, PIN_EXT_I2C_SCL);
  delay(50);  // Bus stabilization delay
  scanI2CBus(extI2C, "After RTC restore");

  // Sync ESP32 system time from RTC if available, else use compile timestamp
  time_t currentTime = rtcMgr->now();
  struct tm buildTime = {};
  if (strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &buildTime) != nullptr) {
    time_t compileTime = mktime(&buildTime);

    // If RTC unset OR older than 2025, use compile timestamp
    if (currentTime < 86400 || currentTime < 1735689600) {  // 2025-01-01
      currentTime = compileTime;
      timeval tv = {currentTime, 0};
      settimeofday(&tv, nullptr);
      rtcMgr->setTime(currentTime);  // Persist to RTC hardware so it survives power cycles
      Serial.print("[BOOT] Time synced to compile, written to RTC: ");
      Serial.println(ctime(&currentTime));
    } else {
      Serial.print("[BOOT] RTC time: ");
      Serial.println(ctime(&currentTime));
    }
  }

  // Log content discovery results
  uint8_t typeCount = contentMgr->getTypeCount();

  // Narrow button count down to what was actually discovered (discoverContent()
  // itself was capped at MAX_BUTTON_TYPES above, so typeCount is already <= 7).
  // Without this, NUM_BUTTON_TYPES stayed at its stale default forever, so any
  // button beyond that count silently produced no events despite having content.
  NUM_BUTTON_TYPES = typeCount;

  Serial.print("[BOOT] Content scan: ");
  Serial.print(typeCount);
  Serial.println(" types");
  for (uint8_t i = 0; i < typeCount; i++) {
    const TypeContent* type = contentMgr->getType(i);
    if (type) {
      Serial.print("[BOOT]   Type");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(type->folderName.c_str());
      Serial.print(" (");
      Serial.print(contentMgr->getVariantCount(i));
      Serial.println(" variants)");
    }
  }
  Serial.println("[BOOT] Ready");

  if (!audioPlayer->initialize()) {
    snprintf(lastError, sizeof(lastError), "AudioPlayer init failed: %s", audioPlayer->getLastError());
    return false;
  }

  scanI2CBus(extI2C, "After AudioPlayer init");

  Serial.println("[DEBUG] Initializing ButtonManager...");
  // CRITICAL WARNING: AudioPlayer initialize() (audioKit->begin()) resets I2C registers and pin mappings.
  // We must re-establish GPIO 18/23 here before initializing ButtonManager. 
  // DO NOT REMOVE THIS CALL or the delay(50)! The delay is electrically required to let lines pull high.
  extI2C.begin(PIN_EXT_I2C_SDA, PIN_EXT_I2C_SCL);
  delay(50);  // Bus stabilization delay
  scanI2CBus(extI2C, "Before ButtonManager init");
  if (!buttonMgr->initialize()) {
    Serial.print("[DEBUG] ButtonManager init warning: ");
    Serial.println(buttonMgr->getLastError());
    Serial.println("[WARNING] Buttons disabled — PCF8574 not responding. Check I2C wiring.");
    // Non-fatal; system continues but buttons won't work
  } else {
    Serial.println("[DEBUG] ButtonManager initialized successfully");
  }

  // Initialize OLED last (after Button/I2C work is done)
  Serial.println("[DEBUG] Initializing OLED display...");
  if (!displayMgr->initialize(nullptr)) {
    Serial.print("[DEBUG] OLED init warning: ");
    Serial.println(displayMgr->getLastError());
    // Non-fatal
  } else {
    Serial.println("[DEBUG] OLED initialized successfully");
  }

  // Initialize easter egg detector
  Serial.println("[DEBUG] Initializing EasterEggDetector...");
  easterEggDetector.reset(new EasterEggDetectorImpl());
  if (!easterEggDetector->initialize(contentMgr->getTypeCount())) {
    Serial.print("[DEBUG] EasterEggDetector init warning: ");
    Serial.println(easterEggDetector->getLastError());
  } else {
    Serial.println("[DEBUG] EasterEggDetector initialized successfully");
    buttonMgr->setEasterEggDetector(easterEggDetector.get());
  }

  // Wire easter egg state to playback controller
  easterEggState.patternLoopIndex[0] = 0;
  playbackCtrl->setEasterEggState(&easterEggState);

  if (!playbackCtrl->initialize()) {
    snprintf(lastError, sizeof(lastError), "PlaybackController init failed: %s", playbackCtrl->getLastError());
    return false;
  }

  return true;
}

void SystemManagerImpl::update() {
  if (webServerMgr) {
    webServerMgr->update();
  }

  // Handle PCF8574 interrupt flag (set by ISR, handled here to avoid I2C in ISR)
  if (buttonMgr) {
    ButtonManagerImpl* btnMgr = static_cast<ButtonManagerImpl*>(buttonMgr.get());
    if (btnMgr && btnMgr->interruptPending) {
      btnMgr->interruptPending = false;
      btnMgr->onPCF8574Interrupt();  // Safe I2C calls in main loop
    }
  }

  // Dequeue button events from interrupt queue (non-blocking). Easter egg
  // pattern check runs right after each release is processed — the same
  // event that drives normal click playback — so tap-based eggs (which need
  // to see "just released, not still held") are evaluated at the moment
  // their window is actually valid, not on some later free-running tick.
  ButtonEvent event;
  while (buttonMgr && buttonMgr->dequeueEvent(event)) {
    handleButtonEvent(event);
    checkAndHandleEasterEgg();
  }

  // Also poll every tick (not just on release) so hold-based eggs, which have
  // no release event to hang off while the button is still down, can fire.
  if (buttonMgr) {
    checkAndHandleEasterEgg();
  }

  processAudioAndMaintenance();
}

void SystemManagerImpl::checkAndHandleEasterEgg() {
  if (buttonMgr) {
    EasterEggPattern pattern = buttonMgr->checkEasterEggPattern();
    if (pattern != EasterEggPattern::NONE) {
      Serial.print("[SYSTEM] Easter Egg Pattern Detected: ");
      Serial.println(getEasterEggPatternName(pattern));

      uint8_t variantIndex = easterEggState.patternLoopIndex[(uint8_t)pattern];
      const char* variantPath = contentMgr->getEasterEggVariantPath(pattern, variantIndex);
      if (variantPath) {
        if (audioPlayer->isPlaying()) {
          audioPlayer->stop();
        }
        if (audioPlayer->playFile(variantPath)) {
          wasPlayingLastFrame = true;
          lastInteractionMs = millis();
          buttonMgr->lockoutButtonsForEasterEgg();
          playbackCtrl->handleEasterEggPattern(pattern);
          
          const char* basename = strrchr(variantPath, '/');
          if (basename) basename++; else basename = variantPath;
          displayMgr->showNowPlaying(basename, 0, 0, true);
        }
      }
    }
  }
}

void SystemManagerImpl::processAudioAndMaintenance() {
  // Process audio playback
  bool isPlaying = audioPlayer->isPlaying();
  if (isPlaying) {
    if (!audioPlayer->process()) {
      // Playback finished
      playbackCtrl->notifyClipFinished();
      wasPlayingLastFrame = false;
      Serial.println("[AUDIO] Playback finished");
      displayMgr->showStandby();
    }
  } else if (wasPlayingLastFrame) {
    // Transitioned from playing to not playing
    wasPlayingLastFrame = false;
    Serial.println("[AUDIO] Stopped");
    displayMgr->showStandby();
  }
  // Re-query rather than reuse the pre-process() `isPlaying` local: process()
  // can call stop() internally when a clip finishes, so that local is stale
  // and would otherwise re-arm wasPlayingLastFrame, firing a spurious
  // "[AUDIO] Stopped" + showStandby() on the very next tick.
  wasPlayingLastFrame = audioPlayer->isPlaying();

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

  // Flush SD queue during quiet window (OLED is on separate bus - no blocking)
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

  // Log button click with type info
  const TypeContent* type = contentMgr->getType(event.typeIndex);
  Serial.print("[BTN] Click: ");
  if (type) {
    String myString = String(type->folderName.c_str());
    Serial.print(myString);
  };
  Serial.print(" (type");
  Serial.print(event.typeIndex);
  Serial.println(")");

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
      if (filePath) {
        // Extract basename for logging
        const char* basename = strrchr(filePath, '/');
        if (basename) basename++; else basename = filePath;

        // Start audio playback FIRST (non-blocking)
        if (audioPlayer->playFile(filePath)) {
          wasPlayingLastFrame = true;
          lastInteractionMs = millis();  // Update interaction timer on playback start

          // Update OLED (separate I2C bus - no blocking)
          displayMgr->showNowPlaying(basename);
        }
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
