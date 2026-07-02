#pragma once

#include <ctime>
#include "Config.h"
#include "ContentTypes.h"

class ConfigLoader {
public:
  virtual ~ConfigLoader() = default;

  // Load and parse config.json for a type
  // Returns true if config found and valid, false if missing/corrupt (use default)
  virtual bool loadTypeConfig(uint8_t typeIndex, const char* folderPath, ModeConfig& outDefaultMode,
                              std::vector<ModeConfig>& outModes) = 0;

  // Determine active mode for current time
  // Returns result with modeIndex and reason
  virtual ModeSelectionResult selectModeForTime(uint8_t typeIndex, time_t currentTime) = 0;

  // Check if a time window is active at a given time
  virtual bool isTimeWindowActive(const TimeWindow& window, time_t currentTime) = 0;

  // Get last config parse error
  virtual const char* getLastError() const = 0;
};

#endif // CONFIG_LOADER_H
