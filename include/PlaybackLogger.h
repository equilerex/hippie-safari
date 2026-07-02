#pragma once

#include <cstdint>
#include <ctime>
#include <vector>
#include "Config.h"
#include "ContentTypes.h"

class PlaybackLogger {
public:
  virtual ~PlaybackLogger() = default;

  // Initialize logger (ensure /logs directory exists)
  virtual bool initialize() = 0;

  // Log an event to queue (flushed to SD when playback inactive)
  virtual void logEvent(const LogEntry& entry) = 0;

  // Convenience methods
  virtual void logTypeSelected(uint8_t typeIndex) = 0;
  virtual void logVariantChanged(uint8_t typeIndex, uint8_t variantIndex) = 0;
  virtual void logPlaybackStarted(uint8_t typeIndex, uint8_t variantIndex) = 0;
  virtual void logPlaybackEnded(uint8_t typeIndex, uint8_t variantIndex) = 0;
  virtual void logModeChanged(uint8_t typeIndex, uint8_t modeIndex) = 0;
  virtual void logSDError(const char* message) = 0;
  virtual void logSDRecovered() = 0;
  virtual void logConfigError(uint8_t typeIndex, const char* message) = 0;
  virtual void logStandbyEntered() = 0;
  virtual void logStandbyExited() = 0;

  // Flush queued events to SD (call when playback inactive)
  virtual bool flushQueue() = 0;

  // Check if queue needs flushing
  virtual bool queueNeedsFlushing() const = 0;

  // Get queue size
  virtual size_t getQueueSize() const = 0;

  // Enable/disable serial debug logging (off by default)
  virtual void setDebugLogging(bool enabled) = 0;

  // Get last logging error
  virtual const char* getLastError() const = 0;
};

#endif // PLAYBACK_LOGGER_H
