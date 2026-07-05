#ifndef BUTTONMANAGERIMPL_H
#define BUTTONMANAGERIMPL_H

#include <cstdint>
#include <cstring>
#include <Wire.h>
#include <PCF8574.h>
#include "../include/ButtonManager.h"
#include "../include/ContentManager.h"
#include "../include/EasterEggDetector.h"

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
  TwoWire* i2cBus = nullptr;  // extI2C bus for raw reads
  EasterEggDetector* easterEggDetector = nullptr;
  ButtonState buttons[MAX_BUTTON_TYPES];
  ButtonEvent lastEvent;
  bool eventDetected = false;
  bool initialized = false;  // Prevent poll() if init failed
  bool pcf8574Ready = false;  // Hardware actually responded
  char lastError[256] = {0};
  uint32_t lastSecretButtonChangeMs = 0;

  // Helper: debounce check
  bool isDebounced(uint32_t now, uint32_t lastChangeMs);

  // Interrupt handler: queue button event
  void queueButtonEvent(const ButtonEvent& event);

  // Check if queued event available
  bool hasQueuedEvent() const;

public:
  volatile bool interruptPending = false;  // Set by ISR on GPIO 19 interrupt

  ButtonManagerImpl(ContentManager* contentMgr, PCF8574* pcf8574, TwoWire* i2cBus = nullptr)
    : contentMgr(contentMgr), pcf8574(pcf8574), i2cBus(i2cBus) {
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
  void setEasterEggDetector(EasterEggDetector* detector) override;
  EasterEggPattern checkEasterEggPattern() override;
  const char* getLastError() const override;

  // New: dequeue button event from interrupt queue
  bool dequeueEvent(ButtonEvent& outEvent);

  // Called from main loop when interruptPending is true (non-blocking I2C)
  void onPCF8574Interrupt();
};

#endif // BUTTONMANAGERIMPL_H
