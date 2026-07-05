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

  // Initialize button ports (PCF8574) and state
  Serial.println("[DEBUG] ButtonManager: Calling pcf8574->begin()...");

  bool pcf8574Ready = pcf8574->begin();
  if (!pcf8574Ready) {
    snprintf(lastError, sizeof(lastError), "PCF8574 not responding at 0x20 — buttons disabled");
    Serial.print("[DEBUG] ButtonManager: ");
    Serial.println(lastError);
    Serial.print("[DEBUG] ButtonManager: PCF8574 lastError code: ");
    Serial.println(pcf8574->lastError());
    // Non-fatal: continue init, but poll will have no real input
  } else {
    Serial.println("[DEBUG] ButtonManager: PCF8574 initialized");
  }
  if (pcf8574Ready) {
    for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
      buttons[i].port = BUTTON_PINS[i];  // Port index (0-7)
      buttons[i].typeIndex = i;
      buttons[i].contentAvailable = contentMgr->typeHasContent(i);
      // PCF8574 defaults to INPUT (HIGH); no explicit pinMode needed
      buttons[i].lastState = pcf8574->read(buttons[i].port);
      buttons[i].lastChangeMs = millis();
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
  // Guard: don't poll if ButtonManager init failed
  if (!initialized) {
    return false;
  }

  uint32_t now = millis();
  eventDetected = false;

  for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
    bool currentState = pcf8574->read(buttons[i].port);

    if (currentState != buttons[i].lastState) {
      // State changed - check debounce
      if (isDebounced(now, buttons[i].lastChangeMs)) {
        buttons[i].lastChangeMs = now;
        buttons[i].lastState = currentState;

        // Falling edge (HIGH to LOW) = button press (active-low on PCF8574)
        if (currentState == LOW && buttons[i].contentAvailable) {
          Serial.print("[DEBUG] ButtonManager: Button ");
          Serial.print(i);
          Serial.println(" pressed");
          lastEvent.typeIndex = i;
          lastEvent.pressTimeMs = now;
          lastEvent.isPress = true;
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
  return (now - lastChangeMs) >= BUTTON_DEBOUNCE_MS;
}

void ButtonManagerImpl::queueButtonEvent(const ButtonEvent& event) {
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
  // Called from ISR when INT pin 19 goes LOW
  // Read all button states and detect presses
  if (!initialized) return;

  uint32_t now = millis();
  for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
    bool currentState = pcf8574->read(buttons[i].port);

    if (currentState != buttons[i].lastState) {
      // State changed - check debounce
      if (isDebounced(now, buttons[i].lastChangeMs)) {
        buttons[i].lastChangeMs = now;
        buttons[i].lastState = currentState;

        // Falling edge (HIGH to LOW) = button press (active-low on PCF8574)
        if (currentState == LOW && buttons[i].contentAvailable) {
          ButtonEvent event;
          event.typeIndex = i;
          event.pressTimeMs = now;
          event.isPress = true;
          queueButtonEvent(event);
        }
      }
    }
  }
}

const char* ButtonManagerImpl::getLastError() const {
  return lastError;
}
