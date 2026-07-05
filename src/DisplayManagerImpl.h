#ifndef DISPLAYMANAGERIMPL_H
#define DISPLAYMANAGERIMPL_H

#include <cstring>
#include <U8g2lib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../include/DisplayManager.h"
#include "../include/Config.h"

// OLED: 128x64 SH1106/SSD1306 at I2C addr 0x3C
// U8g2 handles both chips automatically
// Using SW_I2C to work with external I2C bus (GPIO18/23)
constexpr uint8_t OLED_ADDR = 0x3C;

class DisplayManagerImpl : public DisplayManager {
private:
  U8G2_SH1106_128X64_NONAME_F_SW_I2C* display = nullptr;
  TwoWire* bus = nullptr;
  bool available = false;
  SemaphoreHandle_t i2cMutex = nullptr;  // Mutex to serialize I2C access
  char lastError[256] = {0};

  // Helper: truncate filename to fit on display
  void truncateFilename(char* dest, size_t maxlen, const char* src);

public:
  DisplayManagerImpl() = default;
  ~DisplayManagerImpl() override { if (display) delete display; }

  void setI2CMutex(SemaphoreHandle_t mutex) { i2cMutex = mutex; }
  bool initialize(TwoWire* bus) override;
  bool isAvailable() const override;
  void showNowPlaying(const char* filename) override;
  void showStandby() override;
  void showDebug(const char* line1, const char* line2 = nullptr) override;
  void clear() override;
  const char* getLastError() const override;
};

#endif // DISPLAYMANAGERIMPL_H
