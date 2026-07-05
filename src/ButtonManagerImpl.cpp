#include <Arduino.h>
#include <cstring>
#include "ButtonManagerImpl.h"
#include "../include/Config.h"

bool ButtonManagerImpl::initialize() {
  if (!contentMgr) {
    snprintf(lastError, sizeof(lastError), "ContentManager not set");
    Serial.println("[DEBUG] ButtonManager: ContentManager not set");
    return false;
  }

  if (!pcf8574) {
    snprintf(lastError, sizeof(lastError), "PCF8574 not set");
    Serial.println("[DEBUG] ButtonManager: PCF8574 not set");
    return false;
  }

  // Initialize button ports (PCF8574) and state using the library
  Serial.println("[DEBUG] ButtonManager: Initializing PCF8574...");

  // begin() performs connection check and writes 0xFF (configures pins as inputs)
  this->pcf8574Ready = pcf8574->begin();

  if (!this->pcf8574Ready) {
    snprintf(lastError, sizeof(lastError), "PCF8574 not responding — buttons disabled");
    Serial.print("[DEBUG] ButtonManager: ");
    Serial.println(lastError);
  } else {
    Serial.println("[DEBUG] ButtonManager: PCF8574 initialized");
  }

  if (this->pcf8574Ready) {
    // Read initial port states
    uint8_t portState = pcf8574->read8();

    for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
      buttons[i].port = BUTTON_PINS[i];  // Port index (0-7)
      buttons[i].typeIndex = i;
      buttons[i].contentAvailable = contentMgr->typeHasContent(i);
      // Extract initial state from port byte
      buttons[i].lastState = (portState >> buttons[i].port) & 1;
      buttons[i].lastChangeMs = millis();
      buttons[i].pressStartMs = millis();
      Serial.print("[DEBUG] ButtonManager: Port ");
      Serial.print(buttons[i].port);
      Serial.print(" initialized, initial state: ");
      Serial.println(buttons[i].lastState);
    }
  } else {
    // PCF8574 not ready: initialize button states but don't read from hardware
    for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
      buttons[i].port = BUTTON_PINS[i];
      buttons[i].typeIndex = i;
      buttons[i].contentAvailable = contentMgr->typeHasContent(i);
      buttons[i].lastState = HIGH;  // Default to unpressed
      buttons[i].lastChangeMs = millis();
    }
  }

  eventDetected = false;
  initialized = true;  // Mark init as complete (PCF8574 present or not)
  Serial.println("[DEBUG] ButtonManager: Initialization complete");
  return true;
}

bool ButtonManagerImpl::poll() {
  // Guard: don't poll if ButtonManager init failed or PCF8574 not ready
  if (!initialized || !pcf8574Ready) {
    return false;
  }

  // Read all 8 ports at once
  uint8_t portState = pcf8574->read8();

  uint32_t now = millis();
  eventDetected = false;

  for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
    bool currentState = (portState >> buttons[i].port) & 1;  // Extract bit for this port

    if (currentState != buttons[i].lastState) {
      // State changed - check debounce
      if (isDebounced(now, buttons[i].lastChangeMs)) {
        buttons[i].lastChangeMs = now;
        buttons[i].lastState = currentState;

        // Rising edge (LOW to HIGH) = button release (active-low on PCF8574)
        if (currentState == HIGH && buttons[i].contentAvailable) {
          Serial.print("[DEBUG] ButtonManager: Button ");
          Serial.print(i);
          Serial.println(" released");
          lastEvent.typeIndex = i;
          lastEvent.pressTimeMs = now;
          lastEvent.isPress = false;
          eventDetected = true;
          return true;  // Event detected
        }
      }
    }
  }

  return false;
}

ButtonEvent ButtonManagerImpl::getLastEvent() const {
  return lastEvent;
}

bool ButtonManagerImpl::isTypeAvailable(uint8_t typeIndex) const {
  if (typeIndex < NUM_BUTTON_TYPES) {
    return buttons[typeIndex].contentAvailable;
  }
  return false;
}

bool ButtonManagerImpl::getButtonPressed(uint8_t typeIndex) const {
  if (typeIndex < NUM_BUTTON_TYPES) {
    // lastState reflects PCF8574 port state (LOW = pressed, active-low)
    return buttons[typeIndex].lastState == LOW;
  }
  return false;
}

bool ButtonManagerImpl::isDebounced(uint32_t now, uint32_t lastChangeMs) {
  return (now - lastChangeMs) >= g_tuning.buttonDebounceMs;
}

void ButtonManagerImpl::queueButtonEvent(const ButtonEvent& event) {
  // Skip events during easter egg lockout
  if (isInEasterEggLockout(millis())) {
    return;
  }

  // Add event to circular queue (drop oldest if full)
  size_t nextTail = (queueTail + 1) % MAX_BUTTON_QUEUE;
  if (nextTail == queueHead) {
    // Queue full - drop oldest
    queueHead = (queueHead + 1) % MAX_BUTTON_QUEUE;
  }
  buttonQueue[queueTail] = event;
  queueTail = nextTail;
}

bool ButtonManagerImpl::hasQueuedEvent() const {
  return queueHead != queueTail;
}

bool ButtonManagerImpl::dequeueEvent(ButtonEvent& outEvent) {
  if (!hasQueuedEvent()) {
    return false;
  }
  outEvent = buttonQueue[queueHead];
  queueHead = (queueHead + 1) % MAX_BUTTON_QUEUE;
  return true;
}

void ButtonManagerImpl::onPCF8574Interrupt() {
  // Called from main loop when INT pin 19 goes LOW
  // Read all button states, feed to detector, queue for playback control
  if (!initialized || !pcf8574Ready) return;

  // Read all 8 ports at once
  uint8_t portState = pcf8574->read8();

  uint32_t now = millis();
  for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
    bool currentState = (portState >> buttons[i].port) & 1;  // Extract bit for this port

    if (currentState != buttons[i].lastState) {
      // State changed - check debounce
      if (isDebounced(now, buttons[i].lastChangeMs)) {
        buttons[i].lastChangeMs = now;
        buttons[i].lastState = currentState;

        Serial.printf("[BTN-EVT] %s btn=%u t=%lu\n", currentState == LOW ? "PRESS  " : "RELEASE",
                      i, (unsigned long)now);

        // Feed to easter egg detector for pattern detection (both press and release).
        // Suppressed during easter egg lockout so button spam while an egg clip
        // is playing can't immediately re-trigger another one.
        if (easterEggDetector && !isInEasterEggLockout(now)) {
          if (currentState == LOW) {
            easterEggDetector->recordPress(i, now);
          } else {
            easterEggDetector->recordRelease(i, now);
          }
        }

        if (currentState == LOW) {
          buttons[i].pressStartMs = now;
        }

        // Queue for playback control (release only — normal track change).
        // A hold longer than g_tuning.buttonClickMaxHoldMs is a hold gesture, not a
        // click, so it must not also trigger normal playback.
        if (currentState == HIGH && buttons[i].contentAvailable) {
          uint32_t heldMs = now - buttons[i].pressStartMs;
          if (heldMs <= g_tuning.buttonClickMaxHoldMs) {
            ButtonEvent event;
            event.typeIndex = i;
            event.pressTimeMs = now;
            event.isPress = false;
            queueButtonEvent(event);
          }
        }
      }
    }
  }

  // Also check secret button on P7 (already read from portState)
  if (pcf8574Ready) {
    bool secretState = (portState >> PIN_EASTER_EGG) & 1;
    static bool lastSecretState = HIGH;
    if (secretState != lastSecretState) {
      if (isDebounced(now, lastSecretButtonChangeMs)) {
        lastSecretButtonChangeMs = now;
        lastSecretState = secretState;

        // Feed to detector
        if (easterEggDetector) {
          if (secretState == LOW) {
            easterEggDetector->recordPress(PIN_EASTER_EGG, now);
          } else {
            easterEggDetector->recordRelease(PIN_EASTER_EGG, now);
          }
        }
      }
    }
  }
}

void ButtonManagerImpl::setEasterEggDetector(EasterEggDetector* detector) {
  easterEggDetector = detector;
}

EasterEggPattern ButtonManagerImpl::checkEasterEggPattern() {
  if (!easterEggDetector) {
    return EasterEggPattern::NONE;
  }

  // Events already fed to detector via interrupt handler (onPCF8574Interrupt)
  // No polling here — just check for detected pattern
  return easterEggDetector->checkPattern();
}

const char* ButtonManagerImpl::getLastError() const {
  return lastError;
}

bool ButtonManagerImpl::isInEasterEggLockout(uint32_t now) const {
  return (int32_t)(easterEggLockoutEndMs - now) > 0;
}

void ButtonManagerImpl::lockoutButtonsForEasterEgg() {
  easterEggLockoutEndMs = millis() + 3000;
}
