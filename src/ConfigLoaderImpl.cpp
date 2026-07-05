#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <ctime>
#include <cstring>
#include "ConfigLoaderImpl.h"
#include "../include/Config.h"

bool ConfigLoaderImpl::loadTypeConfig(uint8_t typeIndex, const char* folderPath,
                                     ModeConfig& outDefaultMode, std::vector<ModeConfig>& outModes) {
  outDefaultMode.modeName = "default";
  outDefaultMode.timeWindows.clear();
  outModes.clear();

  // Build path to config.json
  char configPath[256];
  snprintf(configPath, sizeof(configPath), "%s/config.json", folderPath);

  File configFile = SD.open(configPath);
  if (!configFile) {
    // No config found - use default only
    snprintf(lastError, sizeof(lastError), "No config.json for type %u", typeIndex);
    return false;  // Not critical; caller will use default
  }

  // Read file into buffer
  size_t fileSize = configFile.size();
  if (fileSize == 0 || fileSize > 16384) {
    snprintf(lastError, sizeof(lastError), "Config file too large or empty");
    configFile.close();
    return false;
  }

  char* jsonBuffer = new char[fileSize + 1];
  if (!jsonBuffer) {
    snprintf(lastError, sizeof(lastError), "Out of memory for config buffer");
    configFile.close();
    return false;
  }

  configFile.read((uint8_t*)jsonBuffer, fileSize);
  jsonBuffer[fileSize] = '\0';
  configFile.close();

  // Parse JSON
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, jsonBuffer);
  delete[] jsonBuffer;

  if (error) {
    snprintf(lastError, sizeof(lastError), "JSON parse error: %s", error.c_str());
    return false;
  }

  // Extract modes array
  if (!doc.containsKey("modes")) {
    snprintf(lastError, sizeof(lastError), "No 'modes' array in config");
    return false;
  }

  JsonArray modesArray = doc["modes"].as<JsonArray>();
  for (JsonObject modeObj : modesArray) {
    ModeConfig mode;
    const char* modeName = modeObj["name"] | "unnamed";
    mode.modeName = modeName;

    // Parse time windows
    if (modeObj.containsKey("timeWindows")) {
      JsonArray windowsArray = modeObj["timeWindows"].as<JsonArray>();
      for (JsonObject windowObj : windowsArray) {
        TimeWindow window;

        // Determine window type and parse accordingly
        if (windowObj.containsKey("startDateTime") && windowObj.containsKey("endDateTime")) {
          window.type = TimeWindowType::ABSOLUTE_EVENT;
          // TODO: Parse ISO 8601 or epoch timestamps
          window.absoluteStartTime = 0;  // Placeholder
          window.absoluteEndTime = 0;
        } else if (windowObj.containsKey("days")) {
          window.type = TimeWindowType::DAY_SPECIFIC;
          JsonArray daysArray = windowObj["days"].as<JsonArray>();
          window.dayMask = 0;
          for (const char* dayStr : daysArray) {
            if (strcmp(dayStr, "monday") == 0) window.dayMask |= (1 << 0);
            else if (strcmp(dayStr, "tuesday") == 0) window.dayMask |= (1 << 1);
            else if (strcmp(dayStr, "wednesday") == 0) window.dayMask |= (1 << 2);
            else if (strcmp(dayStr, "thursday") == 0) window.dayMask |= (1 << 3);
            else if (strcmp(dayStr, "friday") == 0) window.dayMask |= (1 << 4);
            else if (strcmp(dayStr, "saturday") == 0) window.dayMask |= (1 << 5);
            else if (strcmp(dayStr, "sunday") == 0) window.dayMask |= (1 << 6);
            else if (strcmp(dayStr, "test") == 0 || strcmp(dayStr, "test_odd") == 0) window.dayMask |= (1 << 7);
          }
          // Parse startTime and endTime (HH:MM format)
          const char* startStr = windowObj["startTime"] | "00:00";
          const char* endStr = windowObj["endTime"] | "00:00";
          sscanf(startStr, "%hhu:%hhu", &window.startHour, &window.startMin);
          sscanf(endStr, "%hhu:%hhu", &window.endHour, &window.endMin);
        } else {
          window.type = TimeWindowType::DAILY_RECURRING;
          const char* startStr = windowObj["startTime"] | "00:00";
          const char* endStr = windowObj["endTime"] | "00:00";
          sscanf(startStr, "%hhu:%hhu", &window.startHour, &window.startMin);
          sscanf(endStr, "%hhu:%hhu", &window.endHour, &window.endMin);
        }

        window.priority = windowObj["priority"] | 0;
        mode.timeWindows.push_back(window);
      }
    }

    outModes.push_back(mode);
  }

  return outModes.size() > 0;
}

ModeSelectionResult ConfigLoaderImpl::selectModeForTime(uint8_t typeIndex, time_t currentTime) {
  ModeSelectionResult result;
  result.modeFound = false;
  result.modeIndex = 0xFF;
  result.modeName = "default";
  result.appliedPriority = -1;
  result.reason = "default mode (no config)";

  if (!contentMgr) {
    result.modeFound = true;
    result.modeIndex = 0;
    return result;
  }

  const TypeContent* type = contentMgr->getType(typeIndex);
  if (!type) {
    result.modeFound = true;
    result.modeIndex = 0;
    return result;
  }

  int highestPriority = -1;
  int selectedModeIdx = -1;

  for (size_t i = 0; i < type->modes.size(); i++) {
    const ModeConfig& mode = type->modes[i];
    bool modeActive = false;
    int modePriority = -1;
    
    for (const auto& window : mode.timeWindows) {
      if (isTimeWindowActive(window, currentTime)) {
        modeActive = true;
        if (window.priority > modePriority) {
          modePriority = window.priority;
        }
      }
    }
    
    if (modeActive) {
      if (modePriority > highestPriority) {
        highestPriority = modePriority;
        selectedModeIdx = i;
      }
    }
  }

  if (selectedModeIdx != -1) {
    result.modeFound = true;
    result.modeIndex = selectedModeIdx + 1; // index 0 is reserved for default mode
    result.modeName = type->modes[selectedModeIdx].modeName;
    result.appliedPriority = highestPriority;
    result.reason = "time window matched";
  } else {
    result.modeFound = true;
    result.modeIndex = 0;
    result.modeName = "default";
    result.appliedPriority = -1;
    result.reason = "no custom mode active";
  }

  return result;
}

bool ConfigLoaderImpl::isTimeWindowActive(const TimeWindow& window, time_t currentTime) {
  struct tm* timeinfo = localtime(&currentTime);

  switch (window.type) {
    case TimeWindowType::ABSOLUTE_EVENT: {
      // Check if current time is between absolute start and end
      return currentTime >= window.absoluteStartTime && currentTime <= window.absoluteEndTime;
    }
    case TimeWindowType::DAY_SPECIFIC: {
      // Check day of week match
      if (!isDayMatch(window, currentTime)) {
        return false;
      }
      // Check time of day
      return isTimeOfDayInRange(timeinfo->tm_hour, timeinfo->tm_min,
                                window.startHour, window.startMin,
                                window.endHour, window.endMin);
    }
    case TimeWindowType::DAILY_RECURRING: {
      return isTimeOfDayInRange(timeinfo->tm_hour, timeinfo->tm_min,
                                window.startHour, window.startMin,
                                window.endHour, window.endMin);
    }
  }

  return false;
}

bool ConfigLoaderImpl::isDayMatch(const TimeWindow& window, time_t currentTime) {
  // Check for test mode (odd minute) flag on bit 7
  if (window.dayMask & (1 << 7)) {
    struct tm* timeinfo = localtime(&currentTime);
    return (timeinfo->tm_min % 2 != 0);
  }

  struct tm* timeinfo = localtime(&currentTime);
  // tm_wday: 0=Sunday, 1=Monday, ..., 6=Saturday
  // window.dayMask: 0=Monday, 1=Tuesday, ..., 6=Sunday
  int dayOfWeek = timeinfo->tm_wday;
  int maskBit = (dayOfWeek == 0) ? 6 : (dayOfWeek - 1);  // Convert to mask order

  return (window.dayMask & (1 << maskBit)) != 0;
}

bool ConfigLoaderImpl::isTimeOfDayInRange(uint8_t currentHour, uint8_t currentMin,
                                         uint8_t startHour, uint8_t startMin,
                                         uint8_t endHour, uint8_t endMin) {
  uint16_t currentMinutes = currentHour * 60 + currentMin;
  uint16_t startMinutes = startHour * 60 + startMin;
  uint16_t endMinutes = endHour * 60 + endMin;

  if (startMinutes < endMinutes) {
    // Normal range (doesn't wrap midnight)
    return currentMinutes >= startMinutes && currentMinutes <= endMinutes;
  } else {
    // Wraps midnight (e.g., 22:00 to 06:00)
    return currentMinutes >= startMinutes || currentMinutes <= endMinutes;
  }
}

const char* ConfigLoaderImpl::getLastError() const {
  return lastError;
}
