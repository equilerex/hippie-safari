#ifndef EASTER_EGG_DETECTOR_H
#define EASTER_EGG_DETECTOR_H

#include <cstdint>
#include <ctime>
#include "Config.h"

class EasterEggDetector {
public:
  virtual ~EasterEggDetector() = default;

  // Initialize detector with button count
  virtual bool initialize(uint8_t numButtons) = 0;

  // Record a button press event
  virtual void recordPress(uint8_t buttonIndex, uint32_t timeMs) = 0;

  // Record a button release event
  virtual void recordRelease(uint8_t buttonIndex, uint32_t timeMs) = 0;

  // Check if a pattern has been detected (call periodically or after every event)
  virtual EasterEggPattern checkPattern() = 0;

  // Reset pattern detection state (call after pattern is handled)
  virtual void reset() = 0;

  // Update timeout for loop index reset (call on any interaction)
  virtual void updateInteractionTime(uint32_t timeMs) = 0;

  // Check if loop indices should reset (5 min silence)
  virtual bool shouldResetLoopIndices(uint32_t timeMs) const = 0;

  // Get last detected pattern
  virtual EasterEggPattern getLastPattern() const = 0;

  // Get error message
  virtual const char* getLastError() const = 0;

  // Debug: log internal state
  virtual void logDebugState() const = 0;
};

#endif // EASTER_EGG_DETECTOR_H
