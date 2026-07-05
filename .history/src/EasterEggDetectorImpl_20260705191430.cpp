#include "EasterEggDetectorImpl.h"

void EasterEggDetectorImpl::addEvent(uint8_t buttonIndex, uint32_t timeMs, bool isPress)
{
  eventHistory[eventHead].buttonIndex = buttonIndex;
  eventHistory[eventHead].timeMs = timeMs;
  eventHistory[eventHead].isPress = isPress;
  eventHead = (eventHead + 1) % MAX_EVENT_HISTORY;

  if (eventCount < MAX_EVENT_HISTORY)
  {
    eventCount++;
  }
}

bool EasterEggDetectorImpl::isValidButton(uint8_t buttonIndex) const
{
  return (buttonIndex < numButtons) || (buttonIndex == PIN_EASTER_EGG);
}

void EasterEggDetectorImpl::pruneOldEvents(uint32_t now)
{
  // Remove events older than window
  while (eventCount > 0)
  {
    uint8_t tailIdx = (eventHead + MAX_EVENT_HISTORY - eventCount) % MAX_EVENT_HISTORY;
    if ((uint32_t)(now - eventHistory[tailIdx].timeMs) <= EVENT_HISTORY_WINDOW_MS)
    {
      break;
    }
    eventCount--;
  }
}

bool EasterEggDetectorImpl::isButtonPressed(uint8_t buttonIndex, uint32_t timeMs) const
{
  // Uses live press state (set in recordPress/recordRelease), not the ring
  // buffer, so a hold longer than EVENT_HISTORY_WINDOW_MS still reports
  // pressed — otherwise long-hold eggs could never reach their own threshold.
  if (buttonIndex >= numButtons)
  {
    return false;
  }
  return currentlyPressed[buttonIndex];
}

uint8_t EasterEggDetectorImpl::getPressedButtonCount(uint32_t timeMs) const
{
  uint8_t count = 0;

  for (uint8_t b = 0; b < numButtons; b++)
  {
    if (isButtonPressed(b, timeMs))
    {
      count++;
    }
  }

  return count;
}

bool EasterEggDetectorImpl::wasPatternRecentlyTriggered(uint32_t now) const
{
  return (uint32_t)(now - lastPatternTimeMs) < PATTERN_COOLDOWN_MS;
}

bool EasterEggDetectorImpl::hasInterferingEvent(uint8_t exceptButton, uint32_t sinceMs, uint32_t now) const
{
  for (uint8_t i = 0; i < eventCount; i++)
  {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent &event = eventHistory[idx];

    if (event.buttonIndex != exceptButton && event.timeMs > sinceMs && event.timeMs <= now)
    {
      return true;
    }
  }

  return false;
}

bool EasterEggDetectorImpl::hasInterferingEvent(const bool *exceptButtons, uint32_t sinceMs, uint32_t now) const
{
  for (uint8_t i = 0; i < eventCount; i++)
  {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent &event = eventHistory[idx];

    if (!exceptButtons[event.buttonIndex] && event.timeMs > sinceMs && event.timeMs <= now)
    {
      return true;
    }
  }

  return false;
}

bool EasterEggDetectorImpl::wasHeldTooLong(uint8_t buttonIndex, uint32_t releaseTimeMs, uint32_t maxHoldMs) const
{
  // Find the press immediately preceding this release for the same button
  uint32_t pressTime = 0;

  for (uint8_t i = 0; i < eventCount; i++)
  {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent &event = eventHistory[idx];

    if (event.buttonIndex == buttonIndex && event.isPress && event.timeMs <= releaseTimeMs)
    {
      pressTime = event.timeMs;
    }
  }

  if (pressTime == 0)
  {
    return false; // no matching press in window — can't judge, don't invalidate
  }

  return (releaseTimeMs - pressTime) > maxHoldMs;
}

bool EasterEggDetectorImpl::initialize(uint8_t numButtons)
{
  if (numButtons == 0 || numButtons > MAX_BUTTON_TYPES)
  {
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

void EasterEggDetectorImpl::recordPress(uint8_t buttonIndex, uint32_t timeMs)
{
  if (!isValidButton(buttonIndex))
  {
    return;
  }

  addEvent(buttonIndex, timeMs, true);
  if (buttonIndex < numButtons)
  {
    currentlyPressed[buttonIndex] = true;
    currentPressStartMs[buttonIndex] = timeMs;
  }
  lastInteractionTime = timeMs;
}

void EasterEggDetectorImpl::recordRelease(uint8_t buttonIndex, uint32_t timeMs)
{
  if (!isValidButton(buttonIndex))
  {
    return;
  }

  addEvent(buttonIndex, timeMs, false);
  if (buttonIndex < numButtons)
  {
    currentlyPressed[buttonIndex] = false;
  }
}

EasterEggPattern EasterEggDetectorImpl::checkPattern()
{
  if (eventCount == 0)
  {
    return EasterEggPattern::NONE;
  }

  uint32_t now = millis(); // real elapsed time, not last-event time (needed for hold patterns)

  // Skip if pattern triggered recently (cooldown)
  if (wasPatternRecentlyTriggered(now))
  {
    return EasterEggPattern::NONE;
  }

  pruneOldEvents(now);

  // Check patterns in priority order
  if (detectSecretButton(now))
  {
    lastPattern = EasterEggPattern::SECRET_BUTTON;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectAscendingSweep(now))
  {
    lastPattern = EasterEggPattern::ASCENDING_SWEEP;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectTeamEffort(now))
  {
    lastPattern = EasterEggPattern::TEAM_EFFORT;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectFastClickPair(now))
  {
    lastPattern = EasterEggPattern::FAST_CLICK_PAIR;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectAllButtonsHeld(now))
  {
    lastPattern = EasterEggPattern::ALL_BUTTONS_HELD;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectMultiHold(now))
  {
    lastPattern = EasterEggPattern::MULTI_HOLD;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectLongHoldSustained(now))
  {
    lastPattern = EasterEggPattern::LONG_HOLD_SUSTAINED;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectHammerSingle(now))
  {
    lastPattern = EasterEggPattern::HAMMER_SINGLE;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectMultiClick(now))
  {
    lastPattern = EasterEggPattern::MULTI_CLICK;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectSosMorse(now))
  {
    lastPattern = EasterEggPattern::SOS_MORSE;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  if (detectChaosBurst(now))
  {
    lastPattern = EasterEggPattern::CHAOS_BURST;
    lastPatternTimeMs = now;
    return lastPattern;
  }

  return EasterEggPattern::NONE;
}

void EasterEggDetectorImpl::reset()
{
  eventHead = 0;
  eventCount = 0;
  lastPattern = EasterEggPattern::NONE;
  lastPatternTimeMs = 0;
}

void EasterEggDetectorImpl::updateInteractionTime(uint32_t timeMs)
{
  lastInteractionTime = timeMs;
}

bool EasterEggDetectorImpl::shouldResetLoopIndices(uint32_t timeMs) const
{
  constexpr uint32_t SILENCE_TIMEOUT_MS = 300000; // 5 min
  return (uint32_t)(timeMs - lastInteractionTime) >= SILENCE_TIMEOUT_MS;
}

EasterEggPattern EasterEggDetectorImpl::getLastPattern() const
{
  return lastPattern;
}

const char *EasterEggDetectorImpl::getLastError() const
{
  return lastError;
}

// ============ Pattern Detection Implementations ============

bool EasterEggDetectorImpl::detectSecretButton(uint32_t now)
{
  // Single press of P7 (PIN_EASTER_EGG)
  for (uint8_t i = 0; i < eventCount; i++)
  {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent &event = eventHistory[idx];

    if (event.buttonIndex == PIN_EASTER_EGG && !event.isPress && event.timeMs > lastPatternTimeMs)
    {
      return true;
    }
  }

  return false;
}

bool EasterEggDetectorImpl::detectAscendingSweep(uint32_t now)
{
  // Strict sequence: 0→1→2→...→N-1, in order, within 2 sec, one button at a
  // time. Any out-of-order press, repeat, skipped button, or simultaneous
  // chord (same/near-same timestamp as the previous step) invalidates it.
  constexpr uint32_t WINDOW_MS = 2000;
  constexpr uint32_t MIN_STEP_GAP_MS = 30; // rules out chorded/simultaneous presses
  uint8_t expectedNext = 0;
  uint32_t startTime = 0;
  uint32_t lastStepTime = 0;

  for (uint8_t i = 0; i < eventCount; i++)
  {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent &event = eventHistory[idx];

    if (!event.isPress || event.buttonIndex >= numButtons || event.timeMs <= lastPatternTimeMs)
    {
      continue;
    }

    if (startTime == 0)
    {
      // Sequence must start with button 0
      if (event.buttonIndex != 0)
      {
        continue;
      }
      startTime = event.timeMs;
      lastStepTime = event.timeMs;
      expectedNext = 1;
      continue;
    }

    if ((uint32_t)(event.timeMs - startTime) > WINDOW_MS)
    {
      return false;
    }

    if (event.buttonIndex != expectedNext)
    {
      return false; // out-of-order, repeated, or skipped button — invalidate
    }

    if ((uint32_t)(event.timeMs - lastStepTime) < MIN_STEP_GAP_MS)
    {
      return false; // chorded/simultaneous press, not a deliberate sweep
    }
    lastStepTime = event.timeMs;

    // Reject sweep taps held past the tap threshold, whether still held or already released
    if (isButtonPressed(event.buttonIndex, now))
    {
      if ((uint32_t)(now - event.timeMs) > MAX_TAP_HOLD_MS)
      {
        return false;
      }
    }
    else
    {
      for (uint8_t j = 0; j < eventCount; j++)
      {
        uint8_t ridx = (eventHead + MAX_EVENT_HISTORY - eventCount + j) % MAX_EVENT_HISTORY;
        const ButtonEvent &rel = eventHistory[ridx];
        if (rel.buttonIndex == event.buttonIndex && !rel.isPress && rel.timeMs >= event.timeMs)
        {
          if ((rel.timeMs - event.timeMs) > MAX_TAP_HOLD_MS)
          {
            return false;
          }
          break;
        }
      }
    }

    expectedNext++;
  }

  return expectedNext == numButtons && expectedNext > 1;
}

bool EasterEggDetectorImpl::detectSosMorse(uint32_t now)
{
  // Morse SOS: 3 short taps, pause, 3 medium, pause, 3 short
  // Simplified: count all taps and check for ~9 in 5 sec
  constexpr uint32_t WINDOW_MS = 5000;

  uint32_t oldestTime = 0;
  uint8_t tapCount = 0;

  for (uint8_t i = 0; i < eventCount; i++)
  {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent &event = eventHistory[idx];

    if (event.isPress)
    {
      continue;
    }

    if (oldestTime == 0)
    {
      oldestTime = event.timeMs;
    }

    if ((uint32_t)(now - event.timeMs) <= WINDOW_MS && event.timeMs > lastPatternTimeMs &&
        !wasHeldTooLong(event.buttonIndex, event.timeMs, MAX_TAP_HOLD_MS))
    {
      tapCount++;
    }
  }

  return tapCount >= 7;
}

bool EasterEggDetectorImpl::detectHammerSingle(uint32_t now)
{
  // Same button 15+ times in 3 sec
  constexpr uint32_t WINDOW_MS = 3000;
  constexpr uint8_t TAP_THRESHOLD = 7;

  for (uint8_t b = 0; b < numButtons; b++)
  {
    uint8_t presses = 0;

    for (uint8_t i = 0; i < eventCount; i++)
    {
      uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
      const ButtonEvent &event = eventHistory[idx];

      if (event.buttonIndex == b && !event.isPress &&
          (uint32_t)(now - event.timeMs) <= WINDOW_MS && event.timeMs > lastPatternTimeMs &&
          !wasHeldTooLong(b, event.timeMs, MAX_TAP_HOLD_MS))
      {
        presses++;
      }
    }

    if (presses >= TAP_THRESHOLD)
    {
      return true;
    }
  }

  return false;
}

bool EasterEggDetectorImpl::detectTeamEffort(uint32_t now)
{
  // All N buttons pressed, all within a 100ms span of each other. Presses are
  // looked up across the full event history window (not just the last 100ms of
  // `now`) since confirming "not still held" requires waiting for releases,
  // which can arrive well after the press cluster itself.
  constexpr uint32_t WINDOW_MS = 150;

  uint8_t buttonsPressed = 0;
  bool pressed[MAX_BUTTON_TYPES] = {false};
  uint32_t minPressTime = now;
  uint32_t maxPressTime = 0;

  for (uint8_t i = 0; i < eventCount; i++)
  {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent &event = eventHistory[idx];

    if (event.isPress && event.buttonIndex < numButtons &&
        (uint32_t)(now - event.timeMs) <= EVENT_HISTORY_WINDOW_MS)
    {
      if (!pressed[event.buttonIndex])
      {
        pressed[event.buttonIndex] = true;
        buttonsPressed++;
        minPressTime = (event.timeMs < minPressTime) ? event.timeMs : minPressTime;
        maxPressTime = (event.timeMs > maxPressTime) ? event.timeMs : maxPressTime;
      }
    }
  }

  if (buttonsPressed != numButtons || numButtons <= 1)
  {
    return false;
  }

  // All required presses must land within the same WINDOW_MS span, not just each be recent
  if ((maxPressTime - minPressTime) > WINDOW_MS)
  {
    return false;
  }

  // Still-held buttons belong to ALL_BUTTONS_HELD's territory, not a tap egg
  for (uint8_t b = 0; b < numButtons; b++)
  {
    if (isButtonPressed(b, now))
    {
      return false;
    }
  }

  return true;
}

bool EasterEggDetectorImpl::detectFastClickPair(uint32_t now)
{
  // Exactly 2 buttons pressed, both within a 100ms span of each other. Presses
  // are looked up across the full event history window (not just the last
  // 100ms of `now`) since confirming "not still held" requires waiting for
  // releases, which can arrive well after the press cluster itself.
  constexpr uint32_t WINDOW_MS = 150;

  uint8_t buttonsPressed = 0;
  bool pressed[MAX_BUTTON_TYPES] = {false};
  uint32_t minPressTime = now;
  uint32_t maxPressTime = 0;

  for (uint8_t i = 0; i < eventCount; i++)
  {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent &event = eventHistory[idx];

    if (event.isPress && event.buttonIndex < numButtons &&
        (uint32_t)(now - event.timeMs) <= EVENT_HISTORY_WINDOW_MS)
    {
      if (!pressed[event.buttonIndex])
      {
        pressed[event.buttonIndex] = true;
        buttonsPressed++;
        minPressTime = (event.timeMs < minPressTime) ? event.timeMs : minPressTime;
        maxPressTime = (event.timeMs > maxPressTime) ? event.timeMs : maxPressTime;
      }
    }
  }

  if (buttonsPressed != 2)
  {
    return false;
  }

  // Both presses must land within the same WINDOW_MS span, not just each be recent
  if ((maxPressTime - minPressTime) > WINDOW_MS)
  {
    return false;
  }

  // Still-held buttons head toward MULTI_HOLD, not a fast tap pair
  for (uint8_t b = 0; b < numButtons; b++)
  {
    if (pressed[b] && isButtonPressed(b, now))
    {
      return false;
    }
  }

  return true;
}

bool EasterEggDetectorImpl::detectLongHoldSustained(uint32_t now)
{
  // Any button held 5+ sec. Uses live press state (currentPressStartMs), not
  // the ring buffer, since a hold this long outlives EVENT_HISTORY_WINDOW_MS
  // and its press event would otherwise be pruned before the threshold hits.
  constexpr uint32_t HOLD_MS = 5000;

  for (uint8_t b = 0; b < numButtons; b++)
  {
    if (!currentlyPressed[b])
    {
      continue;
    }

    if ((uint32_t)(now - currentPressStartMs[b]) >= HOLD_MS)
    {
      if (hasInterferingEvent(b, currentPressStartMs[b], now))
      {
        continue;
      }
      return true;
    }
  }

  return false;
}

bool EasterEggDetectorImpl::detectMultiHold(uint32_t now)
{
  // 2+ buttons held simultaneously 3+ sec — hold duration counts from the
  // LAST button to join, since only then are all of them simultaneously held.
  // Buttons must also join within JOIN_WINDOW_MS of each other, otherwise
  // this is just two unrelated holds and not a deliberate multi-hold gesture.
  constexpr uint32_t HOLD_MS = 3000;
  constexpr uint32_t JOIN_WINDOW_MS = 1000;

  uint8_t pressedCount = getPressedButtonCount(now);

  if (pressedCount >= 2)
  {
    uint32_t minPressTime = now;
    uint32_t maxPressTime = 0;
    bool heldSet[MAX_BUTTON_TYPES] = {false};

    for (uint8_t b = 0; b < numButtons; b++)
    {
      if (currentlyPressed[b])
      {
        heldSet[b] = true;
        minPressTime = (currentPressStartMs[b] < minPressTime) ? currentPressStartMs[b] : minPressTime;
        maxPressTime = (currentPressStartMs[b] > maxPressTime) ? currentPressStartMs[b] : maxPressTime;
      }
    }

    if ((maxPressTime - minPressTime) > JOIN_WINDOW_MS)
    {
      return false;
    }

    if (hasInterferingEvent(heldSet, minPressTime, now))
    {
      return false;
    }

    return (uint32_t)(now - maxPressTime) >= HOLD_MS;
  }

  return false;
}

bool EasterEggDetectorImpl::detectAllButtonsHeld(uint32_t now)
{
  // All N buttons held 5+ sec — same "last joiner" and join-window logic as MULTI_HOLD
  constexpr uint32_t HOLD_MS = 5000;
  constexpr uint32_t JOIN_WINDOW_MS = 1000;

  uint8_t pressedCount = getPressedButtonCount(now);

  if (pressedCount == numButtons && numButtons > 1)
  {
    uint32_t minPressTime = now;
    uint32_t maxPressTime = 0;

    for (uint8_t b = 0; b < numButtons; b++)
    {
      minPressTime = (currentPressStartMs[b] < minPressTime) ? currentPressStartMs[b] : minPressTime;
      maxPressTime = (currentPressStartMs[b] > maxPressTime) ? currentPressStartMs[b] : maxPressTime;
    }

    if ((maxPressTime - minPressTime) > JOIN_WINDOW_MS)
    {
      return false;
    }

    // All buttons are part of the held set here, so no external interference is possible
    return (uint32_t)(now - maxPressTime) >= HOLD_MS;
  }

  return false;
}

bool EasterEggDetectorImpl::detectChaosBurst(uint32_t now)
{
  // 12+ random presses (mixed buttons) in 4 sec
  constexpr uint32_t WINDOW_MS = 3000;
  constexpr uint8_t TAP_THRESHOLD = 10;

  uint8_t presses = 0;

  for (uint8_t i = 0; i < eventCount; i++)
  {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent &event = eventHistory[idx];

    if (!event.isPress && (uint32_t)(now - event.timeMs) <= WINDOW_MS && event.timeMs > lastPatternTimeMs)
    {
      presses++;
    }
  }

  return presses >= TAP_THRESHOLD;
}

bool EasterEggDetectorImpl::detectMultiClick(uint32_t now)
{
  // 4+ presses of 2+ different buttons in 2 sec
  constexpr uint32_t WINDOW_MS = 2000;
  constexpr uint8_t TAP_THRESHOLD = 4;

  uint8_t presses = 0;
  bool buttonsSeen[MAX_BUTTON_TYPES] = {false};
  uint8_t uniqueButtons = 0;

  for (uint8_t i = 0; i < eventCount; i++)
  {
    uint8_t idx = (eventHead + MAX_EVENT_HISTORY - eventCount + i) % MAX_EVENT_HISTORY;
    const ButtonEvent &event = eventHistory[idx];

    if (!event.isPress && event.buttonIndex < numButtons &&
        (uint32_t)(now - event.timeMs) <= WINDOW_MS && event.timeMs > lastPatternTimeMs &&
        !wasHeldTooLong(event.buttonIndex, event.timeMs, MAX_TAP_HOLD_MS))
    {
      presses++;

      if (!buttonsSeen[event.buttonIndex])
      {
        buttonsSeen[event.buttonIndex] = true;
        uniqueButtons++;
      }
    }
  }

  return presses >= TAP_THRESHOLD && uniqueButtons >= 2;
}

void EasterEggDetectorImpl::logDebugState() const
{
  Serial.print("[EGG-DBG] Events: ");
  Serial.print(eventCount);
  Serial.print("/");
  Serial.print(MAX_EVENT_HISTORY);
  Serial.print(" | Cooldown: ");
  if (wasPatternRecentlyTriggered(millis()))
  {
    Serial.print("ACTIVE (");
    Serial.print(PATTERN_COOLDOWN_MS - (millis() - lastPatternTimeMs));
    Serial.print("ms left)");
  }
  else
  {
    Serial.print("READY");
  }
  Serial.print(" | LastPattern: ");
  Serial.println(getEasterEggPatternName(lastPattern));
}
