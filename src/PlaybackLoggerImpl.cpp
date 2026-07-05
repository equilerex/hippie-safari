#include <Arduino.h>
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
  return true;
}

void PlaybackLoggerImpl::logEvent(const LogEntry& entry) {
  if (eventQueue.size() >= MAX_LOG_QUEUE_SIZE) {
    // Queue full - drop oldest event (shouldn't normally happen)
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
    Serial.print("LOG: type=");
    Serial.print((uint8_t)entry.eventType);
    Serial.print(" typeIdx=");
    Serial.print(entry.typeIndex);
    Serial.print(" varIdx=");
    Serial.println(entry.variantIndex);
  }
}

void PlaybackLoggerImpl::logTypeSelected(uint8_t typeIndex) {
  LogEntry entry = {time(nullptr), EventType::TYPE_SELECTED, typeIndex, 0xFF, 0xFF, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::logVariantChanged(uint8_t typeIndex, uint8_t variantIndex) {
  LogEntry entry = {time(nullptr), EventType::VARIANT_CHANGED, typeIndex, variantIndex, 0xFF, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::logPlaybackStarted(uint8_t typeIndex, uint8_t variantIndex) {
  LogEntry entry = {time(nullptr), EventType::PLAYBACK_STARTED, typeIndex, variantIndex, 0xFF, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::logPlaybackEnded(uint8_t typeIndex, uint8_t variantIndex) {
  LogEntry entry = {time(nullptr), EventType::PLAYBACK_ENDED, typeIndex, variantIndex, 0xFF, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::logModeChanged(uint8_t typeIndex, uint8_t modeIndex) {
  LogEntry entry = {time(nullptr), EventType::MODE_CHANGED, typeIndex, 0xFF, modeIndex, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::logSDError(const char* message) {
  LogEntry entry = {time(nullptr), EventType::SD_ERROR, 0xFF, 0xFF, 0xFF, message};
  logEvent(entry);
}

void PlaybackLoggerImpl::logSDRecovered() {
  LogEntry entry = {time(nullptr), EventType::SD_RECOVERED, 0xFF, 0xFF, 0xFF, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::logConfigError(uint8_t typeIndex, const char* message) {
  LogEntry entry = {time(nullptr), EventType::CONFIG_ERROR, typeIndex, 0xFF, 0xFF, message};
  logEvent(entry);
}

void PlaybackLoggerImpl::logStandbyEntered() {
  LogEntry entry = {time(nullptr), EventType::STANDBY_ENTERED, 0xFF, 0xFF, 0xFF, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::logStandbyExited() {
  LogEntry entry = {time(nullptr), EventType::STANDBY_EXITED, 0xFF, 0xFF, 0xFF, nullptr};
  logEvent(entry);
}

void PlaybackLoggerImpl::getCurrentLogPath(char* outPath, size_t maxLen, time_t currentTime) {
  struct tm* timeinfo = localtime(&currentTime);
  snprintf(outPath, maxLen, "%s/%04d-%02d-%02d.json", PATH_LOGS_ROOT,
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
}

void PlaybackLoggerImpl::formatEventJSON(const LogEntry& entry, char* outJSON, size_t maxLen) {
  snprintf(outJSON, maxLen,
           "{\"ts\":%ld,\"evt\":%u,\"type\":%u,\"var\":%u,\"mode\":%u}\n",
           entry.timestamp, (uint8_t)entry.eventType, entry.typeIndex,
           entry.variantIndex, entry.modeIndex);
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
