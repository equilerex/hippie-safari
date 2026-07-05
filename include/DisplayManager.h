#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Wire.h>

class DisplayManager {
public:
  virtual ~DisplayManager() = default;

  // Initialize OLED on given I2C bus; non-fatal if OLED absent
  virtual bool initialize(TwoWire* bus) = 0;

  // Check if OLED is available
  virtual bool isAvailable() const = 0;

  // Display now-playing filename (basename only) with optional progress
  virtual void showNowPlaying(const char* filename, size_t bytesRead = 0, size_t totalBytes = 0) = 0;

  // Display standby state
  virtual void showStandby() = 0;

  // Display debug message(s) — up to 2 lines
  virtual void showDebug(const char* line1, const char* line2 = nullptr) = 0;

  // Clear display
  virtual void clear() = 0;

  // Get last error message
  virtual const char* getLastError() const = 0;
};

#endif // DISPLAYMANAGER_H
