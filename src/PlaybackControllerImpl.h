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

  PlaybackState currentState;
  std::vector<uint8_t> variantIndices;  // Per-type index storage
  std::vector<uint8_t> currentModeIndices;  // Per-type current mode
  bool inStandby = false;
  char lastError[256] = {0};

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

private:
  // Helper: get next variant index for type (wraps around)
  uint8_t getNextVariantIndex(uint8_t typeIndex);
};


#endif // PLAYBACKCONTROLLERIMPL_H
