#include <Arduino.h>
#include <cstring>
#include "ButtonManagerImpl.h"
#include "../include/Config.h"

void ButtonManagerImpl::readAllChips(uint8_t (&portState)[NUM_PCF8574_CHIPS]) const {
  for (uint8_t c = 0; c < NUM_PCF8574_CHIPS; c++) {
    portState[c] = chipReady[c] ? pcf8574[c]->read8() : 0xFF;  // Default to unpressed (active-low)
  }
}

bool ButtonManagerImpl::initialize() {
  if (!contentMgr) {
    snprintf(lastError, sizeof(lastError), "ContentManager not set");
    Serial.println("[DEBUG] ButtonManager: ContentManager not set");
    return false;
  }

  // Initialize each PCF8574 chip (begin() performs a connection check and
  // writes 0xFF, configuring pins as inputs)
  bool anyChipReady = false;
  for (uint8_t c = 0; c < NUM_PCF8574_CHIPS; c++) {
    if (!pcf8574[c]) {
      Serial.printf("[DEBUG] ButtonManager: PCF8574 chip %u not set\n", c);
      chipReady[c] = false;
      continue;
    }
    Serial.printf("[DEBUG] ButtonManager: Initializing PCF8574 chip %u (0x%02X)...\n", c, PCF8574_ADDRESSES[c]);
    chipReady[c] = pcf8574[c]->begin();
    if (!chipReady[c]) {
      Serial.printf("[DEBUG] ButtonManager: PCF8574 chip %u not responding\n", c);
    } else {
      Serial.printf("[DEBUG] ButtonManager: PCF8574 chip %u initialized\n", c);
      anyChipReady = true;
    }
  }

  this->pcf8574Ready = anyChipReady;
  if (!anyChipReady) {
    snprintf(lastError, sizeof(lastError), "No PCF8574 chips responding — buttons disabled");
    Serial.print("[DEBUG] ButtonManager: ");
    Serial.println(lastError);
  }

  uint8_t portState[NUM_PCF8574_CHIPS];
  readAllChips(portState);

  for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
    buttons[i].chipIndex = BUTTON_CHIP_INDEX[i];
    buttons[i].port = BUTTON_PORT_INDEX[i];
    buttons[i].typeIndex = i;
    buttons[i].contentAvailable = contentMgr->typeHasContent(i) && chipReady[buttons[i].chipIndex];
    // Extract initial state from this button's chip's port byte (defaults to
    // unpressed/HIGH via readAllChips() if that particular chip isn't ready)
    buttons[i].lastState = (portState[buttons[i].chipIndex] >> buttons[i].port) & 1;
    buttons[i].lastChangeMs = millis();
    buttons[i].pressStartMs = millis();
    Serial.printf("[DEBUG] ButtonManager: Chip %u Port %u initialized, initial state: %u\n",
                  buttons[i].chipIndex, buttons[i].port, buttons[i].lastState);
  }

  eventDetected = false;
  initialized = true;  // Mark init as complete (PCF8574 present or not)
  Serial.println("[DEBUG] ButtonManager: Initialization complete");
  return true;
}

bool ButtonManagerImpl::poll() {
  // Guard: don't poll if ButtonManager init failed or no chip is ready
  if (!initialized || !pcf8574Ready) {
    return false;
  }

  uint8_t portState[NUM_PCF8574_CHIPS];
  readAllChips(portState);

  uint32_t now = millis();
  eventDetected = false;

  for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
    if (!chipReady[buttons[i].chipIndex]) continue;
    bool currentState = (portState[buttons[i].chipIndex] >> buttons[i].port) & 1;

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
  // Called from main loop when INT pin 19 goes LOW. All chips' open-drain
  // INT lines are wired together, so any one of them can have pulled the
  // shared line low — read every ready chip and figure out which bit(s)
  // changed, rather than assuming it was a particular chip.
  if (!initialized || !pcf8574Ready) return;

  uint8_t portState[NUM_PCF8574_CHIPS];
  readAllChips(portState);

  uint32_t now = millis();
  for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
    if (!chipReady[buttons[i].chipIndex]) continue;
    bool currentState = (portState[buttons[i].chipIndex] >> buttons[i].port) & 1;

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

  // Also check secret button on chip 0 / P7 (already read from portState).
  // Note: PIN_EASTER_EGG here is the logical index fed to the detector, not
  // a bit position — the physical bit comes from EASTER_EGG_CHIP/EASTER_EGG_PORT.
  if (chipReady[EASTER_EGG_CHIP]) {
    bool secretState = (portState[EASTER_EGG_CHIP] >> EASTER_EGG_PORT) & 1;
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
