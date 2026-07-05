#ifndef BUTTONMANAGER_H
#define BUTTONMANAGER_H


#include <cstdint>
#include <functional>
#include "Config.h"
#include "ContentTypes.h"
#include "EasterEggDetector.h"

class ButtonManager {
public:
  virtual ~ButtonManager() = default;

  // Initialize button pins and debounce logic
  virtual bool initialize() = 0;

  // Poll buttons in main loop - returns true if event detected
  virtual bool poll() = 0;

  // Get last detected button event
  // Only valid if poll() returned true
  virtual ButtonEvent getLastEvent() const = 0;

  // Check if a button type is available (has content)
  virtual bool isTypeAvailable(uint8_t typeIndex) const = 0;

  // Set callback for when button event occurs (future feature for ISR)
  // virtual void setEventCallback(std::function<void(const ButtonEvent&)> cb) = 0;

  // Get current state of a button (for debugging)
  virtual bool getButtonPressed(uint8_t typeIndex) const = 0;

  // Easter egg detection
  virtual void setEasterEggDetector(EasterEggDetector* detector) = 0;
  virtual EasterEggPattern checkEasterEggPattern() = 0;

  // Lockout button presses for 3 sec after easter egg triggered
  virtual void lockoutButtonsForEasterEgg() = 0;

  // Get last error
  virtual const char* getLastError() const = 0;
};


#endif // BUTTONMANAGER_H
