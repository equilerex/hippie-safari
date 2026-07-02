#pragma once

#include <ctime>
#include "../include/ConfigLoader.h"

class ConfigLoaderImpl : public ConfigLoader {
private:
  char lastError[256] = {0};

  // Helper: parse time window from JSON object
  bool parseTimeWindow(const char* jsonStr, TimeWindow& outWindow);

  // Helper: check if day matches (for day-specific windows)
  bool isDayMatch(const TimeWindow& window, time_t currentTime);

  // Helper: check if time of day falls in window (handles midnight wrap)
  bool isTimeOfDayInRange(uint8_t currentHour, uint8_t currentMin,
                          uint8_t startHour, uint8_t startMin,
                          uint8_t endHour, uint8_t endMin);

public:
  ConfigLoaderImpl() = default;
  ~ConfigLoaderImpl() override = default;

  bool loadTypeConfig(uint8_t typeIndex, const char* folderPath, ModeConfig& outDefaultMode,
                      std::vector<ModeConfig>& outModes) override;
  ModeSelectionResult selectModeForTime(uint8_t typeIndex, time_t currentTime) override;
  bool isTimeWindowActive(const TimeWindow& window, time_t currentTime) override;
  const char* getLastError() const override;
};

#endif // CONFIG_LOADER_IMPL_H
