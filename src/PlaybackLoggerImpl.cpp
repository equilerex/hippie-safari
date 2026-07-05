#include <SD.h>
#include <ctime>
#include <cstring>
#include "PlaybackLoggerImpl.h"
#include "../include/Config.h"

bool PlaybackLoggerImpl::initialize() {
  // Ensure /logs directory exists
  if (!SD.exists(PATH_LOGS_ROOT)) {
    if (!SD.mkdir(PATH_LOGS_ROOT)) {
      snprintf(lastError, sizeof(lastError), "Failed to create %s directory", PATH_LOGS_ROOT);
      return false;
    }
  }

  lastFlushTime = time(nullptr);
  lastLogDate = 0;

  // Write keymap on init
  writeKeymap();

  return true;
}

void PlaybackLoggerImpl::logEvent(const LogEntry& entry) {
  if (eventQueue.size() >= MAX_LOG_QUEUE_SIZE) {
    eventQueue.pop();
  }

  QueuedEvent qEvent;
  qEvent.entry = entry;
  if (entry.details) {
    strncpy(qEvent.details, entry.details, sizeof(qEvent.details) - 1);
    qEvent.details[sizeof(qEvent.details) - 1] = '\0';
  } else {
    qEvent.details[0] = '\0';
  }

  eventQueue.push(qEvent);

  if (debugLoggingEnabled) {
    Serial.print("LOG: evt=");
    Serial.print((uint8_t)entry.eventType);
    Serial.print(" type=");
    Serial.print(entry.typeName ? entry.typeName : "?");
    Serial.print(" var=");
    Serial.println(entry.variantName ? entry.variantName : "?");
  }
}

uint32_t PlaybackLoggerImpl::startSession(uint32_t timeSinceLastMs) {
  uint32_t sessionId = nextSessionId++;
  SessionState state;
  state.sessionId = sessionId;
  state.startTime = time(nullptr);
  state.lastEventTime = state.startTime;
  state.context.timeSinceLastMs = timeSinceLastMs;
  activeSessions[sessionId] = state;

  if (debugLoggingEnabled) {
    Serial.print("[LOG] Session started: ");
    Serial.println(sessionId);
  }

  return sessionId;
}

void PlaybackLoggerImpl::endSession(uint32_t sessionId, const char* typeName, const char* variantName,
                                    uint32_t expectedDurationMs, uint32_t actualDurationMs, bool completedFully) {
  auto it = activeSessions.find(sessionId);
  if (it == activeSessions.end()) {
    return;
  }

  SessionState& session = it->second;
  strncpy(session.typeName, typeName ? typeName : "", sizeof(session.typeName) - 1);
  strncpy(session.variantName, variantName ? variantName : "", sizeof(session.variantName) - 1);
  session.expectedDurationMs = expectedDurationMs;
  session.intent = detectIntent(session, actualDurationMs, completedFully);

  LogEntry entry = {time(nullptr), EventType::PLAYBACK_SESSION, sessionId, session.intent,
                    typeName, variantName, nullptr, expectedDurationMs, actualDurationMs, completedFully, session.context, nullptr};
  logEvent(entry);

  activeSessions.erase(it);

  if (debugLoggingEnabled) {
    Serial.print("[LOG] Session ended: ");
    Serial.print(sessionId);
    Serial.print(" intent=");
    Serial.println((uint8_t)session.intent);
  }
}

void PlaybackLoggerImpl::updateSessionContext(uint32_t sessionId, const PlaybackSessionContext& ctx) {
  auto it = activeSessions.find(sessionId);
  if (it != activeSessions.end()) {
    it->second.context = ctx;
  }
}

PlaybackIntent PlaybackLoggerImpl::detectIntent(const SessionState& session, uint32_t actualDurationMs, bool completedFully) {
  if (completedFully && actualDurationMs > 5000) {
    return PlaybackIntent::ENGAGED;
  }
  if (actualDurationMs < 2000 && session.context.variantCycleCount > 2) {
    return PlaybackIntent::EXPLORING;
  }
  if (actualDurationMs < 2000) {
    return PlaybackIntent::REJECTED;
  }
  if (actualDurationMs >= 5000 && actualDurationMs < session.expectedDurationMs * 0.8) {
    return PlaybackIntent::EXPLORING;
  }
  return PlaybackIntent::ENGAGED;
}

void PlaybackLoggerImpl::logPlaybackStarted(const char* typeName, const char* variantName, const char* modeName, uint32_t expectedDurationMs, uint32_t sessionId) {
  LogEntry entry = {time(nullptr), EventType::PLAYBACK_STARTED, sessionId, PlaybackIntent::UNKNOWN,
                    typeName, variantName, modeName, expectedDurationMs, 0, false, {0, 0, 0, false, 0}, nullptr};
  logEvent(entry);
  lastInteractionTime = entry.timestamp;
}

void PlaybackLoggerImpl::logPlaybackEnded(const char* typeName, const char* variantName, const char* modeName, uint32_t actualDurationMs, bool completedFully, uint32_t sessionId) {
  LogEntry entry = {time(nullptr), EventType::PLAYBACK_ENDED, sessionId, PlaybackIntent::UNKNOWN,
                    typeName, variantName, modeName, 0, actualDurationMs, completedFully, {0, 0, 0, false, 0}, nullptr};
  logEvent(entry);
  lastInteractionTime = entry.timestamp;
}

void PlaybackLoggerImpl::logVariantChanged(const char* typeName, const char* variantName, uint32_t sessionId) {
  LogEntry entry = {time(nullptr), EventType::VARIANT_CHANGED, sessionId, PlaybackIntent::UNKNOWN,
                    typeName, variantName, nullptr, 0, 0, false, {0, 0, 0, false, 0}, nullptr};
  logEvent(entry);
  lastInteractionTime = entry.timestamp;
}

void PlaybackLoggerImpl::logTypeSelected(const char* typeName, uint32_t sessionId) {
  LogEntry entry = {time(nullptr), EventType::TYPE_SELECTED, sessionId, PlaybackIntent::UNKNOWN,
                    typeName, nullptr, nullptr, 0, 0, false, {0, 0, 0, false, 0}, nullptr};
  logEvent(entry);
  lastInteractionTime = entry.timestamp;
}

void PlaybackLoggerImpl::logModeChanged(const char* typeName, const char* modeName, uint32_t sessionId) {
  LogEntry entry = {time(nullptr), EventType::MODE_CHANGED, sessionId, PlaybackIntent::UNKNOWN,
                    typeName, nullptr, modeName, 0, 0, false, {0, 0, 0, false, 0}, nullptr};
  logEvent(entry);
  lastInteractionTime = entry.timestamp;
}

void PlaybackLoggerImpl::logSDError(const char* message) {
  LogEntry entry = {time(nullptr), EventType::SD_ERROR, 0, PlaybackIntent::UNKNOWN,
                    nullptr, nullptr, nullptr, 0, 0, false, {0, 0, 0, false, 0}, message};
  logEvent(entry);
}

void PlaybackLoggerImpl::logSDRecovered() {
  LogEntry entry = {time(nullptr), EventType::SD_RECOVERED, 0, PlaybackIntent::UNKNOWN,
                    nullptr, nullptr, nullptr, 0, 0, false, {0, 0, 0, false, 0}, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::logConfigError(const char* typeName, const char* message) {
  LogEntry entry = {time(nullptr), EventType::CONFIG_ERROR, 0, PlaybackIntent::UNKNOWN,
                    typeName, nullptr, nullptr, 0, 0, false, {0, 0, 0, false, 0}, message};
  logEvent(entry);
}

void PlaybackLoggerImpl::logStandbyEntered() {
  LogEntry entry = {time(nullptr), EventType::STANDBY_ENTERED, 0, PlaybackIntent::UNKNOWN,
                    nullptr, nullptr, nullptr, 0, 0, false, {0, 0, 0, false, 0}, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::logStandbyExited() {
  LogEntry entry = {time(nullptr), EventType::STANDBY_EXITED, 0, PlaybackIntent::UNKNOWN,
                    nullptr, nullptr, nullptr, 0, 0, false, {0, 0, 0, false, 0}, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::logContentDiscovered(int typeCount, int totalVariants) {
  char msg[64];
  snprintf(msg, sizeof(msg), "types=%d, variants=%d", typeCount, totalVariants);
  LogEntry entry = {time(nullptr), EventType::STANDBY_EXITED, 0, PlaybackIntent::UNKNOWN,
                    nullptr, nullptr, nullptr, 0, 0, false, {0, 0, 0, false, 0}, msg};
  logEvent(entry);
}

void PlaybackLoggerImpl::getCurrentLogPath(char* outPath, size_t maxLen, time_t currentTime) {
  struct tm* timeinfo = localtime(&currentTime);
  snprintf(outPath, maxLen, "%s/%04d-%02d-%02d.json", PATH_LOGS_ROOT,
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
}

void PlaybackLoggerImpl::formatEventJSON(const LogEntry& entry, char* outJSON, size_t maxLen) {
  char base[768];
  snprintf(base, sizeof(base),
           "{\"ts\":%ld,\"evt\":%u,\"sid\":%u",
           entry.timestamp, (uint8_t)entry.eventType, entry.sessionId);

  if (entry.intent != PlaybackIntent::UNKNOWN) {
    snprintf(base + strlen(base), sizeof(base) - strlen(base), ",\"intent\":%u", (uint8_t)entry.intent);
  }
  if (entry.typeName) {
    snprintf(base + strlen(base), sizeof(base) - strlen(base), ",\"type\":\"%s\"", entry.typeName);
  }
  if (entry.variantName) {
    snprintf(base + strlen(base), sizeof(base) - strlen(base), ",\"var\":\"%s\"", entry.variantName);
  }
  if (entry.modeName) {
    snprintf(base + strlen(base), sizeof(base) - strlen(base), ",\"mode\":\"%s\"", entry.modeName);
  }
  if (entry.expectedDurationMs > 0) {
    snprintf(base + strlen(base), sizeof(base) - strlen(base), ",\"expDur\":%u", entry.expectedDurationMs);
  }
  if (entry.actualDurationMs > 0) {
    snprintf(base + strlen(base), sizeof(base) - strlen(base), ",\"actDur\":%u", entry.actualDurationMs);
  }
  snprintf(base + strlen(base), sizeof(base) - strlen(base), ",\"complete\":%s", entry.completedFully ? "true" : "false");

  if (entry.eventType == EventType::PLAYBACK_SESSION) {
    snprintf(base + strlen(base), sizeof(base) - strlen(base),
             ",\"ctx\":{\"presses\":%u,\"cycles\":%u,\"timeSinceLast\":%u,\"modeChanged\":%s,\"interruptMs\":%u}",
             entry.context.pressCount, entry.context.variantCycleCount, entry.context.timeSinceLastMs,
             entry.context.modeChanged ? "true" : "false", entry.context.interruptionDelayMs);
  }

  if (entry.details) {
    snprintf(base + strlen(base), sizeof(base) - strlen(base), ",\"msg\":\"%s\"", entry.details);
  }
  snprintf(base + strlen(base), sizeof(base) - strlen(base), "}\n");

  strncpy(outJSON, base, maxLen - 1);
  outJSON[maxLen - 1] = '\0';
}

bool PlaybackLoggerImpl::flushQueue() {
  if (eventQueue.empty()) {
    return true;
  }

  time_t now = time(nullptr);
  char logPath[256];
  getCurrentLogPath(logPath, sizeof(logPath), now);

  // Open file ONCE, write all events, close once (batch write)
  File logFile = SD.open(logPath, FILE_APPEND);
  if (!logFile) {
    snprintf(lastError, sizeof(lastError), "Cannot open log file: %s", logPath);
    return false;
  }

  bool success = true;
  size_t eventsWritten = 0;
  while (!eventQueue.empty()) {
    QueuedEvent qEvent = eventQueue.front();

    char jsonLine[256];
    formatEventJSON(qEvent.entry, jsonLine, sizeof(jsonLine));

    size_t written = logFile.print(jsonLine);
    if (written == 0) {
      snprintf(lastError, sizeof(lastError), "Failed to write to log file: %s", logPath);
      success = false;
      break;  // Stop flushing on first error
    }

    eventsWritten++;
    eventQueue.pop();
  }

  logFile.close();

  if (success && debugLoggingEnabled) {
    Serial.print("[LOG] Flushed ");
    Serial.print(eventsWritten);
    Serial.println(" events to SD");
  }

  if (success) {
    lastFlushTime = now;
  }

  return success;
}

bool PlaybackLoggerImpl::appendToLogFile(const char* logPath, const LogEntry& entry) {
  // Legacy single-event write (kept for backwards compat, but inefficient)
  // For bulk writes, use flushQueue() instead
  File logFile = SD.open(logPath, FILE_APPEND);
  if (!logFile) {
    snprintf(lastError, sizeof(lastError), "Cannot open log file: %s", logPath);
    return false;
  }

  char jsonLine[256];
  formatEventJSON(entry, jsonLine, sizeof(jsonLine));

  size_t written = logFile.print(jsonLine);
  logFile.close();

  return written > 0;
}

bool PlaybackLoggerImpl::queueNeedsFlushing() const {
  return eventQueue.size() > 0;
}

size_t PlaybackLoggerImpl::getQueueSize() const {
  return eventQueue.size();
}

void PlaybackLoggerImpl::setDebugLogging(bool enabled) {
  debugLoggingEnabled = enabled;
}

const char* PlaybackLoggerImpl::getLastError() const {
  return lastError;
}

void PlaybackLoggerImpl::writeKeymap() {
  File kmFile = SD.open("/logs/keymap.json", FILE_WRITE);
  if (!kmFile) {
    snprintf(lastError, sizeof(lastError), "Cannot write keymap");
    return;
  }

  kmFile.println("{");
  kmFile.println("  \"eventTypes\": {");
  kmFile.println("    \"0\": \"TYPE_SELECTED\",");
  kmFile.println("    \"1\": \"VARIANT_CHANGED\",");
  kmFile.println("    \"2\": \"PLAYBACK_STARTED\",");
  kmFile.println("    \"3\": \"PLAYBACK_ENDED\",");
  kmFile.println("    \"4\": \"MODE_CHANGED\",");
  kmFile.println("    \"5\": \"SD_ERROR\",");
  kmFile.println("    \"6\": \"SD_RECOVERED\",");
  kmFile.println("    \"7\": \"CONFIG_ERROR\",");
  kmFile.println("    \"8\": \"STANDBY_ENTERED\",");
  kmFile.println("    \"9\": \"STANDBY_EXITED\",");
  kmFile.println("    \"10\": \"PLAYBACK_SESSION\"");
  kmFile.println("  },");
  kmFile.println("  \"intents\": {");
  kmFile.println("    \"0\": \"UNKNOWN\",");
  kmFile.println("    \"1\": \"ENGAGED\",");
  kmFile.println("    \"2\": \"EXPLORING\",");
  kmFile.println("    \"3\": \"REJECTED\",");
  kmFile.println("    \"4\": \"ERROR\"");
  kmFile.println("  },");
  kmFile.println("  \"schema\": \"ts=unix_epoch, evt=eventType, sid=sessionId, intent=userIntent, type=typeName, var=variantName, mode=modeName, ctx={presses,cycles,timeSinceLast,modeChanged,interruptMs}\"");
  kmFile.println("}");

  kmFile.close();

  if (debugLoggingEnabled) {
    Serial.println("[LOG] Keymap written to /logs/keymap.json");
  }
}

void PlaybackLoggerImpl::checkSessionBoundaries() {
  time_t now = time(nullptr);
  constexpr time_t SESSION_SILENCE_TIMEOUT_S = 300;  // 5 min silence = session end

  auto it = activeSessions.begin();
  while (it != activeSessions.end()) {
    if ((now - it->second.lastEventTime) > SESSION_SILENCE_TIMEOUT_S) {
      // Auto-close silent session
      activeSessions.erase(it++);
    } else {
      ++it;
    }
  }
}
