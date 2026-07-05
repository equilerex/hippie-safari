#include "DisplayManagerImpl.h"
#include <Arduino.h>
#include <string.h>

void DisplayManagerImpl::truncateFilename(char* dest, size_t maxlen, const char* src) {
  if (!src) {
    dest[0] = '\0';
    return;
  }

  // Extract basename (after last /)
  const char* basename = strrchr(src, '/');
  if (basename) {
    basename++;  // Skip the /
  } else {
    basename = src;
  }

  // Strip extension (.wav, .mp3, etc.)
  size_t len = strlen(basename);
  const char* ext = strrchr(basename, '.');
  if (ext) {
    len = ext - basename;
  }

  // Truncate to maxlen
  if (len > maxlen - 1) {
    len = maxlen - 1;
  }

  strncpy(dest, basename, len);
  dest[len] = '\0';
}

bool DisplayManagerImpl::initialize(TwoWire* bus) {
  this->bus = bus;

  // Use SW_I2C with GPIO 5 (SDA) / GPIO 22 (SCL) for separate OLED bus
  Serial.println("[DEBUG] DisplayManager: Initializing with SW_I2C on GPIO5/22...");
  Serial.println("[DEBUG] DisplayManager: Creating U8G2_SH1106 with GPIO5=SDA, GPIO22=SCL");

  display = new U8G2_SH1106_128X64_NONAME_F_SW_I2C(U8G2_R0, 22, 5, U8X8_PIN_NONE);

  Serial.println("[DEBUG] DisplayManager: U8G2 object created");
  Serial.println("[DEBUG] DisplayManager: Calling display->begin()...");

  display->begin();

  Serial.println("[DEBUG] DisplayManager: display->begin() completed");
  available = true;

  Serial.println("[DEBUG] DisplayManager: Clearing buffer and writing boot text...");
  display->clearBuffer();
  display->setFont(u8g2_font_6x10_tf);
  display->drawStr(0, 10, "Hippy Safari");
  display->sendBuffer();

  Serial.println("[DEBUG] DisplayManager: Display initialized successfully");

  return true;
}

bool DisplayManagerImpl::isAvailable() const {
  return available;
}

void DisplayManagerImpl::showNowPlaying(const char* filename) {
  if (!available || !display) return;

  char basename[21];
  truncateFilename(basename, sizeof(basename), filename);

  Serial.print("[DEBUG] OLED: showNowPlaying - ");
  Serial.println(basename);

  if (i2cMutex) xSemaphoreTake(i2cMutex, portMAX_DELAY);
  {
    display->clearBuffer();
    display->setFont(u8g2_font_6x10_tf);
    display->drawStr(0, 10, "Playing:");
    display->drawStr(0, 25, basename);
    display->sendBuffer();
    // Yield to let button polling happen during I2C transaction
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  if (i2cMutex) xSemaphoreGive(i2cMutex);

  Serial.println("[DEBUG] OLED: sendBuffer() completed");
}

void DisplayManagerImpl::showStandby() {
  if (!available || !display) return;

  Serial.println("[DEBUG] OLED: showStandby");

  if (i2cMutex) xSemaphoreTake(i2cMutex, portMAX_DELAY);
  {
    display->clearBuffer();
    display->setFont(u8g2_font_6x10_tf);
    display->drawStr(0, 10, "Standby");
    display->sendBuffer();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  if (i2cMutex) xSemaphoreGive(i2cMutex);

  Serial.println("[DEBUG] OLED: sendBuffer() completed");
}

void DisplayManagerImpl::showDebug(const char* line1, const char* line2) {
  if (!available || !display) return;

  if (i2cMutex) xSemaphoreTake(i2cMutex, portMAX_DELAY);
  {
    display->clearBuffer();
    display->setFont(u8g2_font_6x10_tf);
    if (line1) {
      display->drawStr(0, 10, line1);
    }
    if (line2) {
      display->drawStr(0, 25, line2);
    }
    display->sendBuffer();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  if (i2cMutex) xSemaphoreGive(i2cMutex);
}

void DisplayManagerImpl::clear() {
  if (!available || !display) return;
  display->clearBuffer();
  display->sendBuffer();
}

const char* DisplayManagerImpl::getLastError() const {
  return lastError;
}
