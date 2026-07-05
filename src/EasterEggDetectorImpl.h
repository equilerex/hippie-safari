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
  static constexpr uint32_t EVENT_HISTORY_WINDOW_MS = 10000;
  static constexpr uint32_t PATTERN_COOLDOWN_MS = 1500;

  ButtonEvent eventHistory[MAX_EVENT_HISTORY];
  uint8_t eventHead = 0;
  uint8_t eventCount = 0;

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
};

#endif // EASTER_EGG_DETECTOR_IMPL_H
