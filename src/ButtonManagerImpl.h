#ifndef BUTTONMANAGERIMPL_H
#define BUTTONMANAGERIMPL_H

#include <cstdint>
#include <cstring>
#include <PCF8574.h>
#include "../include/ButtonManager.h"
#include "../include/ContentManager.h"

class ButtonManagerImpl : public ButtonManager {
private:
  struct ButtonState {
    uint8_t port;  // PCF8574 port index (0-7), was ESP32 GPIO
    uint8_t typeIndex;
    bool lastState;
    uint32_t lastChangeMs;
    bool contentAvailable;
  };

  ContentManager* contentMgr = nullptr;
  PCF8574* pcf8574 = nullptr;
  ButtonState buttons[NUM_BUTTON_TYPES];
  ButtonEvent lastEvent;
  bool eventDetected = false;
  bool initialized = false;  // Prevent poll() if init failed
  char lastError[256] = {0};

  // Helper: debounce check
  bool isDebounced(uint32_t now, uint32_t lastChangeMs);

public:
  ButtonManagerImpl(ContentManager* contentMgr, PCF8574* pcf8574)
    : contentMgr(contentMgr), pcf8574(pcf8574) {
    lastEvent.typeIndex = 0xFF;
    lastEvent.pressTimeMs = 0;
    lastEvent.isPress = false;
  }
  ~ButtonManagerImpl() override = default;

  bool initialize() override;
  bool poll() override;
  ButtonEvent getLastEvent() const override;
  bool isTypeAvailable(uint8_t typeIndex) const override;
  bool getButtonPressed(uint8_t typeIndex) const override;
  const char* getLastError() const override;
};

#endif // BUTTONMANAGERIMPL_H
