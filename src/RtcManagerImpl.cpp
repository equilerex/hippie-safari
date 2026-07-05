#include "RtcManagerImpl.h"
#include <Arduino.h>

bool RtcManagerImpl::initialize(TwoWire* bus) {
  if (!bus) {
    snprintf(lastError, sizeof(lastError), "I2C bus is null");
    return false;
  }

  if (!rtc.begin(bus)) {
    snprintf(lastError, sizeof(lastError), "DS3231 RTC not found at 0x68");
    available = false;
    if (logger) {
      logger->logSDError(lastError);
    }
    return false;  // Non-fatal; system continues without RTC
  }

  // Cache current time for later const reads
  lastCachedTime = rtc.now().unixtime();
  available = true;

  if (rtc.lostPower()) {
    snprintf(lastError, sizeof(lastError), "RTC lost power; time not set");
    available = false;
    if (logger) {
      logger->logSDError(lastError);
    }
    // Don't set time here; user must sync RTC via external mechanism
    // Mode selection will fall back to "default" until RTC is synced
  }

  return true;
}

bool RtcManagerImpl::isAvailable() const {
  return available;
}

time_t RtcManagerImpl::now() const {
  if (!available) {
    return 0;  // Fall back to epoch-era time (no mode windows will match)
  }
  // Return cached time from last successful read during initialize()
  // (RTC_DS3231::now() is not const, so we cache on init)
  return lastCachedTime;
}

const char* RtcManagerImpl::getLastError() const {
  return lastError;
}
