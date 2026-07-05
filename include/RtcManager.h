#ifndef RTCMANAGER_H
#define RTCMANAGER_H

#include <ctime>
#include <Wire.h>

class RtcManager {
public:
  virtual ~RtcManager() = default;

  // Initialize RTC on given I2C bus; non-fatal if RTC absent
  virtual bool initialize(TwoWire* bus) = 0;

  // Check if RTC is available and synced
  virtual bool isAvailable() const = 0;

  // Get current time as Unix epoch (time_t); returns 0 if unavailable
  virtual time_t now() const = 0;

  // Get last error message
  virtual const char* getLastError() const = 0;
};

#endif // RTCMANAGER_H
