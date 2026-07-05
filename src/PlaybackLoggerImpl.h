#ifndef PLAYBACKLOGGERIMPL_H
#define PLAYBACKLOGGERIMPL_H


#include <vector>
#include <queue>
#include <map>
#include <cstdint>
#include <ctime>
#include <cstring>
#include "../include/PlaybackLogger.h"
#include "../include/Config.h"

class PlaybackLoggerImpl : public PlaybackLogger {
private:
  struct QueuedEvent {
    LogEntry entry;
    char details[128];
  };

  struct SessionState {
    uint32_t sessionId;
    time_t startTime;
    time_t lastEventTime;
    PlaybackIntent intent = PlaybackIntent::UNKNOWN;
    PlaybackSessionContext context = {0, 0, 0, false, 0};
    char typeName[64] = {0};
    char variantName[256] = {0};
    uint32_t expectedDurationMs = 0;
  };

  std::queue<QueuedEvent> eventQueue;
  std::map<uint32_t, SessionState> activeSessions;
  time_t lastFlushTime = 0;
  time_t lastLogDate = 0;
  time_t lastInteractionTime = 0;
  bool debugLoggingEnabled = false;
  char lastError[256] = {0};
  uint32_t nextSessionId = 1;

  // Helper: get current log file path (rotates daily)
  void getCurrentLogPath(char* outPath, size_t maxLen, time_t currentTime);

  // Helper: append event to log file
  bool appendToLogFile(const char* logPath, const LogEntry& entry);

  // Helper: format event as JSON
  void formatEventJSON(const LogEntry& entry, char* outJSON, size_t maxLen);

  // Helper: write keymap to SD
  void writeKeymap();

  // Helper: detect intent from session timing and interaction patterns
  PlaybackIntent detectIntent(const SessionState& session, uint32_t actualDurationMs, bool completedFully);

  // Helper: check for session boundaries (standby, silence timeout)
  void checkSessionBoundaries();

public:
  PlaybackLoggerImpl() = default;
  ~PlaybackLoggerImpl() override = default;

  bool initialize() override;
  void logEvent(const LogEntry& entry) override;

  // Session management
  uint32_t startSession(uint32_t timeSinceLastMs = 0) override;
  void endSession(uint32_t sessionId, const char* typeName, const char* variantName,
                  uint32_t expectedDurationMs, uint32_t actualDurationMs, bool completedFully) override;
  void updateSessionContext(uint32_t sessionId, const PlaybackSessionContext& ctx) override;

  // Logging methods with session support
  void logPlaybackStarted(const char* typeName, const char* variantName, const char* modeName, uint32_t expectedDurationMs, uint32_t sessionId = 0) override;
  void logPlaybackEnded(const char* typeName, const char* variantName, const char* modeName, uint32_t actualDurationMs, bool completedFully, uint32_t sessionId = 0) override;
  void logVariantChanged(const char* typeName, const char* variantName, uint32_t sessionId = 0) override;
  void logTypeSelected(const char* typeName, uint32_t sessionId = 0) override;
  void logModeChanged(const char* typeName, const char* modeName, uint32_t sessionId = 0) override;
  void logSDError(const char* message) override;
  void logSDRecovered() override;
  void logConfigError(const char* typeName, const char* message) override;
  void logStandbyEntered() override;
  void logStandbyExited() override;
  void logContentDiscovered(int typeCount, int totalVariants) override;

  bool flushQueue() override;
  bool queueNeedsFlushing() const override;
  size_t getQueueSize() const override;
  void setDebugLogging(bool enabled) override;
  const char* getLastError() const override;
};


#endif // PLAYBACKLOGGERIMPL_H
