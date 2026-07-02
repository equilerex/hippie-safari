#include <Arduino.h>
#include <cstring>
#include "ButtonManagerImpl.h"
#include "../include/Config.h"

bool ButtonManagerImpl::initialize() {
  if (!contentMgr) {
    snprintf(lastError, sizeof(lastError), "ContentManager not set");
    return false;
  }

  // Initialize button pins and state
  for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
    buttons[i].pin = BUTTON_PINS[i];
    buttons[i].typeIndex = i;
    buttons[i].contentAvailable = contentMgr->typeHasContent(i);
    pinMode(buttons[i].pin, INPUT_PULLUP);
    buttons[i].lastState = digitalRead(buttons[i].pin);
    buttons[i].lastChangeMs = millis();
  }

  eventDetected = false;
  return true;
}

bool ButtonManagerImpl::poll() {
  uint32_t now = millis();
  eventDetected = false;

  for (uint8_t i = 0; i < NUM_BUTTON_TYPES; i++) {
    bool currentState = digitalRead(buttons[i].pin);

    if (currentState != buttons[i].lastState) {
      // State changed - check debounce
      if (isDebounced(now, buttons[i].lastChangeMs)) {
        buttons[i].lastChangeMs = now;
        buttons[i].lastState = currentState;

        // Rising edge (HIGH) to falling edge (LOW) = button press
        if (currentState == LOW && buttons[i].contentAvailable) {
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
    return buttons[typeIndex].lastState == LOW;
  }
  return false;
}

bool ButtonManagerImpl::isDebounced(uint32_t now, uint32_t lastChangeMs) {
  return (now - lastChangeMs) >= BUTTON_DEBOUNCE_MS;
}

const char* ButtonManagerImpl::getLastError() const {
  return lastError;
}
