#ifndef DISPLAYMANAGERIMPL_H
#define DISPLAYMANAGERIMPL_H

#include <cstring>
#include <U8g2lib.h>
#include "../include/DisplayManager.h"
#include "../include/Config.h"

// OLED: 128x64 SH1106 at I2C addr 0x3C
// Using SW_I2C (bit-banging) on GPIO 5 (SDA) / GPIO 22 (SCL)
// Separate from extI2C (GPIO 18/23), no mutex needed
constexpr uint8_t OLED_ADDR = 0x3C;

class DisplayManagerImpl : public DisplayManager {
private:
  U8G2_SH1106_128X64_NONAME_F_SW_I2C* display = nullptr;
  TwoWire* bus = nullptr;
  bool available = false;
  char lastError[256] = {0};

  // Helper: truncate filename to fit on display
  void truncateFilename(char* dest, size_t maxlen, const char* src);

public:
  DisplayManagerImpl() = default;
  ~DisplayManagerImpl() override { if (display) delete display; }

  bool initialize(TwoWire* bus) override;
  bool isAvailable() const override;
  void showNowPlaying(const char* filename, size_t bytesRead = 0, size_t totalBytes = 0) override;
  void showStandby() override;
  void showDebug(const char* line1, const char* line2 = nullptr) override;
  void clear() override;
  const char* getLastError() const override;
};

#endif // DISPLAYMANAGERIMPL_H
