#ifndef EASTER_EGG_DETECTOR_IMPL_H
#define EASTER_EGG_DETECTOR_IMPL_H

#include <Arduino.h>
#include "../include/EasterEggDetector.h"

class EasterEggDetectorImpl : public EasterEggDetector {
private:
  struct ButtonEvent {
    uint8_t buttonIndex = 0;
    uint32_t timeMs = 0;
    bool isPress = false;
  };

  static constexpr uint8_t MAX_EVENT_HISTORY = 64;

  ButtonEvent eventHistory[MAX_EVENT_HISTORY];
  uint8_t eventHead = 0;
  uint8_t eventCount = 0;

  // Live press state, independent of the ring buffer's EVENT_HISTORY_WINDOW_MS
  // staleness — a button held longer than that window must still register as
  // pressed, otherwise long-hold eggs can never reach their own threshold.
  bool currentlyPressed[MAX_BUTTON_TYPES] = {false};
  uint32_t currentPressStartMs[MAX_BUTTON_TYPES] = {0};

  // Hold-based eggs (LONG_HOLD_SUSTAINED, MULTI_HOLD, ALL_BUTTONS_HELD) must
  // fire once per continuous hold, not once per cooldown window — otherwise
  // holding a button past the threshold replays the clip on a loop. Marks
  // which buttons were "consumed" by a hold-egg firing; cleared on release.
  bool holdConsumed[MAX_BUTTON_TYPES] = {false};

  uint8_t numButtons = 0;
  EasterEggPattern lastPattern = EasterEggPattern::NONE;
  uint32_t lastPatternTimeMs = 0;
  uint32_t lastInteractionTime = 0;
  char lastError[256] = {0};

  void addEvent(uint8_t buttonIndex, uint32_t timeMs, bool isPress);
  void pruneOldEvents(uint32_t now);
  bool isValidButton(uint8_t buttonIndex) const;

  // Pattern detection helpers
  bool detectSecretButton(uint32_t now);
  bool detectAscendingSweep(uint32_t now);
  bool detectSosMorse(uint32_t now);
  bool detectHammerSingle(uint32_t now);
  bool detectTeamEffort(uint32_t now);
  bool detectFastClickPair(uint32_t now);
  bool detectLongHoldSustained(uint32_t now);
  bool detectMultiHold(uint32_t now);
  bool detectAllButtonsHeld(uint32_t now);
  bool detectChaosBurst(uint32_t now);
  bool detectMultiClick(uint32_t now);

  // Utility: check if button is currently pressed (scans history in-place)
  bool isButtonPressed(uint8_t buttonIndex, uint32_t timeMs) const;

  // Utility: count pressed buttons at a given time
  uint8_t getPressedButtonCount(uint32_t timeMs) const;

  bool wasPatternRecentlyTriggered(uint32_t now) const;

  // Purity gate: true if any button other than exceptButton had a press/release
  // event in (sinceMs, now] — used to invalidate hold-based eggs on interference
  bool hasInterferingEvent(uint8_t exceptButton, uint32_t sinceMs, uint32_t now) const;

  // Same, but excludes an arbitrary set of buttons (for multi-button holds)
  bool hasInterferingEvent(const bool* exceptButtons, uint32_t sinceMs, uint32_t now) const;

  // True if the press matching this release (same button, nearest prior press)
  // was held longer than maxHoldMs — used to invalidate tap-based eggs
  bool wasHeldTooLong(uint8_t buttonIndex, uint32_t releaseTimeMs, uint32_t maxHoldMs) const;

public:
  EasterEggDetectorImpl() = default;
  ~EasterEggDetectorImpl() override = default;

  bool initialize(uint8_t numButtons) override;
  void recordPress(uint8_t buttonIndex, uint32_t timeMs) override;
  void recordRelease(uint8_t buttonIndex, uint32_t timeMs) override;
  EasterEggPattern checkPattern() override;
  void reset() override;
  void updateInteractionTime(uint32_t timeMs) override;
  bool shouldResetLoopIndices(uint32_t timeMs) const override;
  EasterEggPattern getLastPattern() const override;
  const char* getLastError() const override;
  void logDebugState() const override;
};

#endif // EASTER_EGG_DETECTOR_IMPL_H
