#ifndef PLAYBACKLOGGER_H
#define PLAYBACKLOGGER_H


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

  // Session management for intent tracking
  virtual uint32_t startSession(uint32_t timeSinceLastMs = 0) = 0;
  virtual void endSession(uint32_t sessionId, const char* typeName, const char* variantName,
                          uint32_t expectedDurationMs, uint32_t actualDurationMs, bool completedFully) = 0;
  virtual void updateSessionContext(uint32_t sessionId, const PlaybackSessionContext& ctx) = 0;

  // Convenience methods with full context
  virtual void logPlaybackStarted(const char* typeName, const char* variantName, const char* modeName, uint32_t expectedDurationMs, uint32_t sessionId = 0) = 0;
  virtual void logPlaybackEnded(const char* typeName, const char* variantName, const char* modeName, uint32_t actualDurationMs, bool completedFully, uint32_t sessionId = 0) = 0;
  virtual void logVariantChanged(const char* typeName, const char* variantName, uint32_t sessionId = 0) = 0;
  virtual void logTypeSelected(const char* typeName, uint32_t sessionId = 0) = 0;
  virtual void logModeChanged(const char* typeName, const char* modeName, uint32_t sessionId = 0) = 0;
  virtual void logSDError(const char* message) = 0;
  virtual void logSDRecovered() = 0;
  virtual void logConfigError(const char* typeName, const char* message) = 0;
  virtual void logStandbyEntered() = 0;
  virtual void logStandbyExited() = 0;
  virtual void logContentDiscovered(int typeCount, int totalVariants) = 0;

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


#endif // PLAYBACKLOGGER_H
