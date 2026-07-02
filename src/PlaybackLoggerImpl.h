#ifndef PLAYBACKLOGGERIMPL_H
#define PLAYBACKLOGGERIMPL_H


#include <vector>
#include <queue>
#include <cstdint>
#include <ctime>
#include "../include/PlaybackLogger.h"
#include "../include/Config.h"

class PlaybackLoggerImpl : public PlaybackLogger {
private:
  struct QueuedEvent {
    LogEntry entry;
    char details[128];
  };

  std::queue<QueuedEvent> eventQueue;
  time_t lastFlushTime = 0;
  time_t lastLogDate = 0;  // Track which day's log file is open
  bool debugLoggingEnabled = false;
  char lastError[256] = {0};

  // Helper: get current log file path (rotates daily)
  void getCurrentLogPath(char* outPath, size_t maxLen, time_t currentTime);

  // Helper: append event to log file
  bool appendToLogFile(const char* logPath, const LogEntry& entry);

  // Helper: format event as JSON
  void formatEventJSON(const LogEntry& entry, char* outJSON, size_t maxLen);

public:
  PlaybackLoggerImpl() = default;
  ~PlaybackLoggerImpl() override = default;

  bool initialize() override;
  void logEvent(const LogEntry& entry) override;
  void logTypeSelected(uint8_t typeIndex) override;
  void logVariantChanged(uint8_t typeIndex, uint8_t variantIndex) override;
  void logPlaybackStarted(uint8_t typeIndex, uint8_t variantIndex) override;
  void logPlaybackEnded(uint8_t typeIndex, uint8_t variantIndex) override;
  void logModeChanged(uint8_t typeIndex, uint8_t modeIndex) override;
  void logSDError(const char* message) override;
  void logSDRecovered() override;
  void logConfigError(uint8_t typeIndex, const char* message) override;
  void logStandbyEntered() override;
  void logStandbyExited() override;
  bool flushQueue() override;
  bool queueNeedsFlushing() const override;
  size_t getQueueSize() const override;
  void setDebugLogging(bool enabled) override;
  const char* getLastError() const override;
};


#endif // PLAYBACKLOGGERIMPL_H
