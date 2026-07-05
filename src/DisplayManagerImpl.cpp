#include "DisplayManagerImpl.h"
#include <Arduino.h>
#include <string.h>

void DisplayManagerImpl::truncateFilename(char *dest, size_t maxlen, const char *src)
{
  if (!src)
  {
    dest[0] = '\0';
    return;
  }

  // Extract basename (after last /)
  const char *basename = strrchr(src, '/');
  if (basename)
  {
    basename++; // Skip the /
  }
  else
  {
    basename = src;
  }

  // Strip extension (.wav, .mp3, etc.)
  size_t len = strlen(basename);
  const char *ext = strrchr(basename, '.');
  if (ext)
  {
    len = ext - basename;
  }

  // Truncate to maxlen
  if (len > maxlen - 1)
  {
    len = maxlen - 1;
  }

  strncpy(dest, basename, len);
  dest[len] = '\0';
}

bool DisplayManagerImpl::initialize(TwoWire *bus)
{
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
  display->setFont(u8g2_font_mystery_quest_24_tf);
  int w = display->getStrWidth("Hippy Safari");
  display->drawStr((128 - w) / 2, 32, "Hippy Safari");
  display->sendBuffer();

  Serial.println("[DEBUG] DisplayManager: Display initialized successfully");

  return true;
}

bool DisplayManagerImpl::isAvailable() const
{
  return available;
}

void DisplayManagerImpl::showNowPlaying(const char *filename, size_t bytesRead, size_t totalBytes)
{
  if (!available || !display)
    return;

  // Mystery Quest 24 characters are wide; truncate to 12 chars to ensure it fits on screen
  char basename[13];
  truncateFilename(basename, sizeof(basename), filename);

  display->clearBuffer();
  display->setFont(u8g2_font_mystery_quest_24_tf);

  // Draw header "Playing:"
  display->drawStr(0, 24, "Playing:");

  // Draw track name
  display->drawStr(0, 52, basename);

  display->sendBuffer();
}

void DisplayManagerImpl::showStandby()
{
  if (!available || !display)
    return;

  Serial.println("[DEBUG] OLED: showStandby");

  display->clearBuffer();
  display->setFont(u8g2_font_mystery_quest_24_tf);
  int w = display->getStrWidth("Standby");
  display->drawStr((128 - w) / 2, 32, "Standby");
  display->sendBuffer();
}

void DisplayManagerImpl::showDebug(const char *line1, const char *line2)
{
  if (!available || !display)
    return;

  display->clearBuffer();
  display->setFont(u8g2_font_mystery_quest_24_tf);
  if (line1)
  {
    int w = display->getStrWidth(line1);
    display->drawStr((128 - w) / 2, 28, line1);
  }
  if (line2)
  {
    int w = display->getStrWidth(line2);
    display->drawStr((128 - w) / 2, 56, line2);
  }
  display->sendBuffer();
}

void DisplayManagerImpl::clear()
{
  if (!available || !display)
    return;
  display->clearBuffer();
  display->sendBuffer();
}

const char *DisplayManagerImpl::getLastError() const
{
  return lastError;
}
