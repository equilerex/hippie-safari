#ifndef PLAYBACKCONTROLLERIMPL_H
#define PLAYBACKCONTROLLERIMPL_H


#include <vector>
#include <cstdint>
#include <ctime>
#include "../include/PlaybackController.h"
#include "../include/ContentManager.h"
#include "../include/ConfigLoader.h"
#include "../include/PlaybackLogger.h"

class PlaybackControllerImpl : public PlaybackController {
private:
  ContentManager* contentMgr = nullptr;
  ConfigLoader* configLoader = nullptr;
  PlaybackLogger* logger = nullptr;
  EasterEggState* eggState = nullptr;

  PlaybackState currentState;
  std::vector<uint8_t> variantIndices;  // Per-type index storage
  std::vector<uint8_t> currentModeIndices;  // Per-type current mode
  bool inStandby = false;
  char lastError[256] = {0};
  uint32_t currentSessionId = 0;  // Active playback session
  time_t lastSessionEndTime = 0;  // Track time since last playback for session context
  EasterEggPattern currentEasterEggPattern = EasterEggPattern::NONE;  // Active easter egg pattern
  uint32_t easterEggStartTimeMs = 0;  // When easter egg started

public:
  PlaybackControllerImpl(ContentManager* contentMgr, ConfigLoader* configLoader, PlaybackLogger* logger)
    : contentMgr(contentMgr), configLoader(configLoader), logger(logger) {}

  ~PlaybackControllerImpl() override = default;

  bool initialize() override;
  PlaybackRequest handleButtonEvent(uint8_t typeIndex, time_t eventTime) override;
  void notifyClipFinished() override;
  const PlaybackState& getCurrentState() const override;
  uint8_t getVariantIndex(uint8_t typeIndex) const override;
  void resetVariantIndex(uint8_t typeIndex) override;
  void setVariantIndex(uint8_t typeIndex, uint8_t index) override;
  const char* getLastError() const override;
  void enterStandby() override;
  void exitStandby() override;
  bool isInStandby() const override;
  void handleEasterEggPattern(EasterEggPattern pattern) override;
  void setEasterEggState(EasterEggState* eggState) override;

private:
  // Helpers: active mode variant filtering and index mapping
  std::vector<std::string> getActiveVariants(uint8_t typeIndex, const char* modeName);
  uint8_t mapFilteredToUnfilteredIndex(uint8_t typeIndex, const char* modeName, uint8_t filteredIndex);
  uint8_t getNextVariantIndex(uint8_t typeIndex, const char* modeName);
};


#endif // PLAYBACKCONTROLLERIMPL_H
