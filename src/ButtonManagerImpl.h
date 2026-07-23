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
    uint8_t chipIndex;  // which PCF8574 chip (0..NUM_PCF8574_CHIPS-1)
    uint8_t port;  // PCF8574 port index (0-7) on that chip
    uint8_t typeIndex;
    bool lastState;
    uint32_t lastChangeMs;
    uint32_t pressStartMs;  // timestamp of most recent press, for hold-duration check on release
    bool contentAvailable;
  };

  static constexpr size_t MAX_BUTTON_QUEUE = 10;
  ButtonEvent buttonQueue[MAX_BUTTON_QUEUE];
  size_t queueHead = 0;
  size_t queueTail = 0;

  ContentManager* contentMgr = nullptr;
  PCF8574* pcf8574[NUM_PCF8574_CHIPS] = {nullptr};
  bool chipReady[NUM_PCF8574_CHIPS] = {false};  // Each chip's hardware responded
  TwoWire* i2cBus = nullptr;  // extI2C bus for raw reads
  EasterEggDetector* easterEggDetector = nullptr;
  ButtonState buttons[MAX_BUTTON_TYPES];
  ButtonEvent lastEvent;
  bool eventDetected = false;
  bool initialized = false;  // Prevent poll() if init failed
  bool pcf8574Ready = false;  // At least one chip responded
  char lastError[256] = {0};
  uint32_t lastSecretButtonChangeMs = 0;
  uint32_t easterEggLockoutEndMs = 0;  // Lockout normal clicks during easter egg audio

  // Helper: debounce check
  bool isDebounced(uint32_t now, uint32_t lastChangeMs);

  // Helper: check if in easter egg lockout period
  bool isInEasterEggLockout(uint32_t now) const;

  // Interrupt handler: queue button event
  void queueButtonEvent(const ButtonEvent& event);

  // Check if queued event available
  bool hasQueuedEvent() const;

  // Read all ready chips' 8-bit port states in one pass (one I2C txn each)
  void readAllChips(uint8_t (&portState)[NUM_PCF8574_CHIPS]) const;

public:
  volatile bool interruptPending = false;  // Set by ISR on GPIO 19 interrupt

  ButtonManagerImpl(ContentManager* contentMgr, PCF8574* chips[NUM_PCF8574_CHIPS], TwoWire* i2cBus = nullptr)
    : contentMgr(contentMgr), i2cBus(i2cBus) {
    for (uint8_t c = 0; c < NUM_PCF8574_CHIPS; c++) {
      this->pcf8574[c] = chips[c];
    }
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
  void lockoutButtonsForEasterEgg() override;
  const char* getLastError() const override;

  // New: dequeue button event from interrupt queue
  bool dequeueEvent(ButtonEvent& outEvent);

  // Called from main loop when interruptPending is true (non-blocking I2C)
  void onPCF8574Interrupt();
};

#endif // BUTTONMANAGERIMPL_H
