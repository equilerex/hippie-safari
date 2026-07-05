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

  static constexpr size_t MAX_BUTTON_QUEUE = 10;
  ButtonEvent buttonQueue[MAX_BUTTON_QUEUE];
  size_t queueHead = 0;
  size_t queueTail = 0;

  ContentManager* contentMgr = nullptr;
  PCF8574* pcf8574 = nullptr;
  ButtonState buttons[NUM_BUTTON_TYPES];
  ButtonEvent lastEvent;
  bool eventDetected = false;
  bool initialized = false;  // Prevent poll() if init failed
  char lastError[256] = {0};

  // Helper: debounce check
  bool isDebounced(uint32_t now, uint32_t lastChangeMs);

  // Interrupt handler: queue button event
  void queueButtonEvent(const ButtonEvent& event);

  // Check if queued event available
  bool hasQueuedEvent() const;

public:
  ButtonManagerImpl(ContentManager* contentMgr, PCF8574* pcf8574)
    : contentMgr(contentMgr), pcf8574(pcf8574) {
    lastEvent.typeIndex = 0xFF;
    lastEvent.pressTimeMs = 0;
    lastEvent.isPress = false;
  }
  ~ButtonManagerImpl() override = default;

  bool initialize() override;
  bool poll() override;  // Deprecated: use interrupt + dequeueEvent() instead
  ButtonEvent getLastEvent() const override;
  bool isTypeAvailable(uint8_t typeIndex) const override;
  bool getButtonPressed(uint8_t typeIndex) const override;
  const char* getLastError() const override;

  // New: dequeue button event from interrupt queue
  bool dequeueEvent(ButtonEvent& outEvent);

  // Called from GPIO interrupt handler on INT pin 19
  void onPCF8574Interrupt();
};

#endif // BUTTONMANAGERIMPL_H
