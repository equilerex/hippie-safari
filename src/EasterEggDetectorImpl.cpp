#include "EasterEggDetectorImpl.h"

void EasterEggDetectorImpl::addEvent(uint8_t buttonIndex, uint32_t timeMs, bool isPress) {
  eventHistory[eventHead].buttonIndex = buttonIndex;
  eventHistory[eventHead].timeMs = timeMs;
  eventHistory[eventHead].isPress = isPress;
  eventHead = (eventHead + 1) % MAX_EVENT_HISTORY;

  if (eventCount < MAX_EVENT_HISTORY) {
    eventCount++;
  }
}

bool EasterEggDetectorImpl::isValidButton(uint8_t buttonIndex) const {
  return (buttonIndex < numButtons) || (buttonIndex == PIN_EASTER_EGG);
}

void EasterEggDetectorImpl::pruneOldEvents(uint32_t now) {
  // Remove events older than window
  while (eventCount > 0) {
    uint8_t tailIdx = (eventHead + MAX_EVENT_HISTORY - eventCount) % MAX_EVENT_HISTORY;
    if ((uint32_t)(now - eventHistory[tailIdx].timeMs) <= EVENT_HISTORY_WINDOW_MS) {
      break;
    }
    eventCount--;
  }
}

bool EasterEggDetectorImpl::isButtonPressed(uint8_t buttonIndex, uint32_t timeMs) const {
  bool pressed = false;

  for (uint8_t i = 0; i < eventCount; i++) {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent& event = eventHistory[idx];

    if (event.buttonIndex == buttonIndex &&
        (uint32_t)(timeMs - event.timeMs) < EVENT_HISTORY_WINDOW_MS) {
      pressed = event.isPress;
    }
  }

  return pressed;
}

uint8_t EasterEggDetectorImpl::getPressedButtonCount(uint32_t timeMs) const {
  uint8_t count = 0;

  for (uint8_t b = 0; b < numButtons; b++) {
    if (isButtonPressed(b, timeMs)) {
      count++;
    }
  }

  return count;
}

bool EasterEggDetectorImpl::wasPatternRecentlyTriggered(uint32_t now) const {
  return (uint32_t)(now - lastPatternTimeMs) < PATTERN_COOLDOWN_MS;
}

bool EasterEggDetectorImpl::initialize(uint8_t numButtons) {
  if (numButtons == 0 || numButtons > MAX_BUTTON_TYPES) {
    snprintf(lastError, sizeof(lastError), "Invalid button count: %u", numButtons);
    return false;
  }

  this->numButtons = numButtons;
  eventHead = 0;
  eventCount = 0;
  lastPattern = EasterEggPattern::NONE;
  lastPatternTimeMs = 0;
  lastInteractionTime = 0;

  return true;
}

void EasterEggDetectorImpl::recordPress(uint8_t buttonIndex, uint32_t timeMs) {
  if (!isValidButton(buttonIndex)) {
    return;
  }

  addEvent(buttonIndex, timeMs, true);
  lastInteractionTime = timeMs;
}

void EasterEggDetectorImpl::recordRelease(uint8_t buttonIndex, uint32_t timeMs) {
  if (!isValidButton(buttonIndex)) {
    return;
  }

  addEvent(buttonIndex, timeMs, false);
}

EasterEggPattern EasterEggDetectorImpl::checkPattern() {
  if (eventCount == 0) {
    return EasterEggPattern::NONE;
  }

  uint32_t now = eventHistory[(eventHead + MAX_EVENT_HISTORY - 1) % MAX_EVENT_HISTORY].timeMs;

  // Skip if pattern triggered recently (cooldown)
  if (wasPatternRecentlyTriggered(now)) {
    return EasterEggPattern::NONE;
  }

  pruneOldEvents(now);

  // Check patterns in priority order
  if (detectSecretButton(now)) {
    lastPattern = EasterEggPattern::SECRET_BUTTON;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectAscendingSweep(now)) {
    lastPattern = EasterEggPattern::ASCENDING_SWEEP;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectTeamEffort(now)) {
    lastPattern = EasterEggPattern::TEAM_EFFORT;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectAllButtonsHeld(now)) {
    lastPattern = EasterEggPattern::ALL_BUTTONS_HELD;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectMultiHold(now)) {
    lastPattern = EasterEggPattern::MULTI_HOLD;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectLongHoldSustained(now)) {
    lastPattern = EasterEggPattern::LONG_HOLD_SUSTAINED;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectHammerSingle(now)) {
    lastPattern = EasterEggPattern::HAMMER_SINGLE;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectMultiClick(now)) {
    lastPattern = EasterEggPattern::MULTI_CLICK;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectSosMorse(now)) {
    lastPattern = EasterEggPattern::SOS_MORSE;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectChaosBurst(now)) {
    lastPattern = EasterEggPattern::CHAOS_BURST;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  return EasterEggPattern::NONE;
}

void EasterEggDetectorImpl::reset() {
  eventHead = 0;
  eventCount = 0;
  lastPattern = EasterEggPattern::NONE;
  lastPatternTimeMs = 0;
}

void EasterEggDetectorImpl::updateInteractionTime(uint32_t timeMs) {
  lastInteractionTime = timeMs;
}

bool EasterEggDetectorImpl::shouldResetLoopIndices(uint32_t timeMs) const {
  constexpr uint32_t SILENCE_TIMEOUT_MS = 300000; // 5 min
  return (uint32_t)(timeMs - lastInteractionTime) >= SILENCE_TIMEOUT_MS;
}

EasterEggPattern EasterEggDetectorImpl::getLastPattern() const {
  return lastPattern;
}

const char* EasterEggDetectorImpl::getLastError() const {
  return lastError;
}

// ============ Pattern Detection Implementations ============

bool EasterEggDetectorImpl::detectSecretButton(uint32_t now) {
  // Single press of P7 (PIN_EASTER_EGG)
  for (uint8_t i = 0; i < eventCount; i++) {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent& event = eventHistory[idx];

    if (event.buttonIndex == PIN_EASTER_EGG && event.isPress) {
      return true;
    }
  }

  return false;
}

bool EasterEggDetectorImpl::detectAscendingSweep(uint32_t now) {
  // Sequence: 0→1→2→...→N within 2 sec
  constexpr uint32_t WINDOW_MS = 2000;
  uint8_t expectedNext = 0;
  uint32_t startTime = 0;

  for (uint8_t i = 0; i < eventCount; i++) {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent& event = eventHistory[idx];

    if (!event.isPress || event.buttonIndex >= numButtons) {
      continue;
    }

    if (startTime == 0) {
      startTime = event.timeMs;
      if (event.buttonIndex == 0) {
        expectedNext = 1;
      }
    }

    if ((uint32_t)(event.timeMs - startTime) > WINDOW_MS) {
      return false;
    }

    if (event.buttonIndex == expectedNext) {
      expectedNext++;
    }
  }

  return expectedNext == numButtons && expectedNext > 1;
}

bool EasterEggDetectorImpl::detectSosMorse(uint32_t now) {
  // Morse SOS: 3 short taps, pause, 3 medium, pause, 3 short
  // Simplified: count all taps and check for ~9 in 5 sec
  constexpr uint32_t WINDOW_MS = 5000;

  uint32_t oldestTime = 0;
  uint8_t tapCount = 0;

  for (uint8_t i = 0; i < eventCount; i++) {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent& event = eventHistory[idx];

    if (!event.isPress) {
      continue;
    }

    if (oldestTime == 0) {
      oldestTime = event.timeMs;
    }

    if ((uint32_t)(now - event.timeMs) <= WINDOW_MS) {
      tapCount++;
    }
  }

  return tapCount >= 7;
}

bool EasterEggDetectorImpl::detectHammerSingle(uint32_t now) {
  // Same button 15+ times in 3 sec
  constexpr uint32_t WINDOW_MS = 3000;
  constexpr uint8_t TAP_THRESHOLD = 15;

  for (uint8_t b = 0; b < numButtons; b++) {
    uint8_t presses = 0;

    for (uint8_t i = 0; i < eventCount; i++) {
      uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
      const ButtonEvent& event = eventHistory[idx];

      if (event.buttonIndex == b && event.isPress &&
          (uint32_t)(now - event.timeMs) <= WINDOW_MS) {
        presses++;
      }
    }

    if (presses >= TAP_THRESHOLD) {
      return true;
    }
  }

  return false;
}

bool EasterEggDetectorImpl::detectTeamEffort(uint32_t now) {
  // All N buttons pressed within 100ms
  constexpr uint32_t WINDOW_MS = 100;

  uint32_t windowStart = now - WINDOW_MS;
  uint8_t buttonsPressed = 0;
  bool pressed[MAX_BUTTON_TYPES] = {false};

  for (uint8_t i = 0; i < eventCount; i++) {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent& event = eventHistory[idx];

    if (event.isPress && event.buttonIndex < numButtons &&
        (uint32_t)(now - event.timeMs) <= WINDOW_MS) {
      if (!pressed[event.buttonIndex]) {
        pressed[event.buttonIndex] = true;
        buttonsPressed++;
      }
    }
  }

  return buttonsPressed == numButtons && numButtons > 1;
}

bool EasterEggDetectorImpl::detectLongHoldSustained(uint32_t now) {
  // Any button held 5+ sec
  constexpr uint32_t HOLD_MS = 5000;

  for (uint8_t b = 0; b < numButtons; b++) {
    uint32_t pressTime = 0;
    uint32_t releaseTime = 0;

    for (uint8_t i = eventCount; i > 0; i--) {
      uint8_t idx = (eventHead + MAX_EVENT_HISTORY - i) % MAX_EVENT_HISTORY;
      const ButtonEvent& event = eventHistory[idx];

      if (event.buttonIndex == b) {
        if (event.isPress && pressTime == 0) {
          pressTime = event.timeMs;
        } else if (!event.isPress) {
          releaseTime = event.timeMs;
        }
      }
    }

    if (pressTime > 0 && (releaseTime == 0 || releaseTime < pressTime)) {
      if ((uint32_t)(now - pressTime) >= HOLD_MS) {
        return true;
      }
    }
  }

  return false;
}

bool EasterEggDetectorImpl::detectMultiHold(uint32_t now) {
  // 2+ buttons held simultaneously 3+ sec
  constexpr uint32_t HOLD_MS = 3000;

  uint8_t pressedCount = getPressedButtonCount(now);

  if (pressedCount >= 2) {
    // Check if all were pressed within recent history
    uint32_t minPressTime = now;

    for (uint8_t b = 0; b < numButtons; b++) {
      if (isButtonPressed(b, now)) {
        for (uint8_t i = 0; i < eventCount; i++) {
          uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
          const ButtonEvent& event = eventHistory[idx];

          if (event.buttonIndex == b && event.isPress) {
            minPressTime = (event.timeMs < minPressTime) ? event.timeMs : minPressTime;
          }
        }
      }
    }

    return (uint32_t)(now - minPressTime) >= HOLD_MS;
  }

  return false;
}

bool EasterEggDetectorImpl::detectAllButtonsHeld(uint32_t now) {
  // All N buttons held 5+ sec
  constexpr uint32_t HOLD_MS = 5000;

  uint8_t pressedCount = getPressedButtonCount(now);

  if (pressedCount == numButtons && numButtons > 1) {
    uint32_t minPressTime = now;

    for (uint8_t b = 0; b < numButtons; b++) {
      for (uint8_t i = 0; i < eventCount; i++) {
        uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
        const ButtonEvent& event = eventHistory[idx];

        if (event.buttonIndex == b && event.isPress) {
          minPressTime = (event.timeMs < minPressTime) ? event.timeMs : minPressTime;
        }
      }
    }

    return (uint32_t)(now - minPressTime) >= HOLD_MS;
  }

  return false;
}

bool EasterEggDetectorImpl::detectChaosBurst(uint32_t now) {
  // 12+ random presses (mixed buttons) in 4 sec
  constexpr uint32_t WINDOW_MS = 4000;
  constexpr uint8_t TAP_THRESHOLD = 12;

  uint8_t presses = 0;

  for (uint8_t i = 0; i < eventCount; i++) {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent& event = eventHistory[idx];

    if (event.isPress && (uint32_t)(now - event.timeMs) <= WINDOW_MS) {
      presses++;
    }
  }

  return presses >= TAP_THRESHOLD;
}

bool EasterEggDetectorImpl::detectMultiClick(uint32_t now) {
  // 4+ presses of 2+ different buttons in 2 sec
  constexpr uint32_t WINDOW_MS = 2000;
  constexpr uint8_t TAP_THRESHOLD = 4;

  uint8_t presses = 0;
  bool buttonsSeen[MAX_BUTTON_TYPES] = {false};
  uint8_t uniqueButtons = 0;

  for (uint8_t i = 0; i < eventCount; i++) {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent& event = eventHistory[idx];

    if (event.isPress && event.buttonIndex < numButtons &&
        (uint32_t)(now - event.timeMs) <= WINDOW_MS) {
      presses++;

      if (!buttonsSeen[event.buttonIndex]) {
        buttonsSeen[event.buttonIndex] = true;
        uniqueButtons++;
      }
    }
  }

  return presses >= TAP_THRESHOLD && uniqueButtons >= 2;
}
