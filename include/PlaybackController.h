#ifndef PLAYBACKCONTROLLER_H
#define PLAYBACKCONTROLLER_H


#include <cstdint>
#include <ctime>
#include "Config.h"
#include "ContentTypes.h"

class PlaybackController {
public:
  virtual ~PlaybackController() = default;

  // Initialize state machine with content and logger references
  virtual bool initialize() = 0;

  // Process button event: determine playback action
  // Returns PlaybackRequest with what to play
  virtual PlaybackRequest handleButtonEvent(uint8_t typeIndex, time_t eventTime) = 0;

  // Report that a clip finished playing
  virtual void notifyClipFinished() = 0;

  // Get current playback state
  virtual const PlaybackState& getCurrentState() const = 0;

  // Get current variant index for a type
  virtual uint8_t getVariantIndex(uint8_t typeIndex) const = 0;

  // Reset variant index for a type (called on mode change)
  virtual void resetVariantIndex(uint8_t typeIndex) = 0;

  // Set variant index manually (rarely used, for debugging)
  virtual void setVariantIndex(uint8_t typeIndex, uint8_t index) = 0;

  // Get last playback error
  virtual const char* getLastError() const = 0;

  // Standby/recovery helpers
  virtual void enterStandby() = 0;
  virtual void exitStandby() = 0;
  virtual bool isInStandby() const = 0;
};


#endif // PLAYBACKCONTROLLER_H
