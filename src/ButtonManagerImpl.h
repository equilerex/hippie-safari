#pragma once

#include <cstdint>
#include <cstring>
#include "../include/ButtonManager.h"
#include "../include/ContentManager.h"

class ButtonManagerImpl : public ButtonManager {
private:
  struct ButtonState {
    uint8_t pin;
    uint8_t typeIndex;
    bool lastState;
    uint32_t lastChangeMs;
    bool contentAvailable;
  };

  ContentManager* contentMgr = nullptr;
  ButtonState buttons[NUM_BUTTON_TYPES];
  ButtonEvent lastEvent;
  bool eventDetected = false;
  char lastError[256] = {0};

  // Helper: debounce check
  bool isDebounced(uint32_t now, uint32_t lastChangeMs);

public:
  ButtonManagerImpl(ContentManager* contentMgr) : contentMgr(contentMgr) {
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

#endif // BUTTON_MANAGER_IMPL_H
