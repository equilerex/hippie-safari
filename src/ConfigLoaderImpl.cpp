#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <ctime>
#include <cstring>
#include "ConfigLoaderImpl.h"
#include "../include/Config.h"

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
      bool active = isTimeWindowActive(window, currentTime);
      if (window.type == TimeWindowType::ABSOLUTE_EVENT) {
        Serial.printf("[MODE-DBG] type=%u mode=%s window(ABSOLUTE start=%ld end=%ld prio=%d) now=%ld active=%d\n",
                      typeIndex, mode.modeName.c_str(),
                      (long)window.absoluteStartTime, (long)window.absoluteEndTime,
                      window.priority, (long)currentTime, active ? 1 : 0);
      } else {
        Serial.printf("[MODE-DBG] type=%u mode=%s window(%s dayMask=0x%02x %02u:%02u-%02u:%02u prio=%d) now=%ld active=%d\n",
                      typeIndex, mode.modeName.c_str(),
                      window.type == TimeWindowType::DAY_SPECIFIC ? "DAY_SPECIFIC" : "DAILY_RECURRING",
                      window.dayMask, window.startHour, window.startMin, window.endHour, window.endMin,
                      window.priority, (long)currentTime, active ? 1 : 0);
      }
      if (active) {
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

  if (startMinutes == endMinutes) {
    // Zero-duration window (e.g. unset "00:00"-"00:00") — never active
    return false;
  }

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
