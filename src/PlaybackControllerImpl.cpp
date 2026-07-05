#include <Arduino.h>
#include <algorithm>
#include <cstring>
#include <SD.h>
#include "PlaybackControllerImpl.h"

bool PlaybackControllerImpl::initialize() {
  if (!contentMgr || !configLoader || !logger) {
    snprintf(lastError, sizeof(lastError), "Dependencies not set");
    return false;
  }

  uint8_t typeCount = contentMgr->getTypeCount();
  if (typeCount == 0) {
    snprintf(lastError, sizeof(lastError), "No types discovered");
    return false;
  }

  // Initialize variant index storage (all start at 0)
  variantIndices.resize(typeCount, 0);
  currentModeIndices.resize(typeCount, 0);

  currentState.currentTypeIndex = 0xFF;  // No type selected yet
  currentState.currentVariantIndex = 0;
  currentState.isPlaying = false;
  currentState.currentModeIndex = 0;

  return true;
}

PlaybackRequest PlaybackControllerImpl::handleButtonEvent(uint8_t typeIndex, time_t eventTime) {
  // If an easter egg was playing, log its interruption
  if (currentEasterEggPattern != EasterEggPattern::NONE) {
    uint32_t durationMs = millis() - easterEggStartTimeMs;
    uint8_t prevIndex = 0;
    if (eggState) {
      auto variants = contentMgr->getEasterEggVariants(currentEasterEggPattern);
      if (!variants.empty()) {
        uint8_t currentIndex = eggState->patternLoopIndex[(uint8_t)currentEasterEggPattern];
        prevIndex = (currentIndex + variants.size() - 1) % variants.size();
      }
    }
    const char* prevPath = contentMgr->getEasterEggVariantPath(currentEasterEggPattern, prevIndex);
    if (!prevPath) prevPath = "unknown.wav";
    const char* prevBasename = strrchr(prevPath, '/');
    if (prevBasename) prevBasename++; else prevBasename = prevPath;

    logger->logEasterEggEnded(currentEasterEggPattern, prevBasename, durationMs, false, currentSessionId);
    currentEasterEggPattern = EasterEggPattern::NONE;
  }

  PlaybackRequest request;
  request.action = PlaybackAction::START_CLIP;
  request.typeIndex = typeIndex;
  request.requestTimeMs = millis();

  if (!contentMgr->typeHasContent(typeIndex)) {
    snprintf(lastError, sizeof(lastError), "Type %u has no content", typeIndex);
    request.action = PlaybackAction::STOP_CLIP;
    return request;
  }

  // Determine active mode for this type
  ModeSelectionResult modeResult = configLoader->selectModeForTime(typeIndex, eventTime);

  // Get type name and mode name early
  const TypeContent* typeContent = contentMgr->getType(typeIndex);
  if (!typeContent) {
    snprintf(lastError, sizeof(lastError), "Type %u not found", typeIndex);
    request.action = PlaybackAction::STOP_CLIP;
    return request;
  }
  const char* typeName = typeContent->folderName.c_str();
  const char* modeName = modeResult.modeFound ? modeResult.modeName.c_str() : "default";

  // Check if mode changed from previous press
  if (modeResult.modeFound && currentModeIndices[typeIndex] != modeResult.modeIndex) {
    // Mode switched: reset variant index
    resetVariantIndex(typeIndex);
    currentModeIndices[typeIndex] = modeResult.modeIndex;
    logger->logModeChanged(typeName, modeName, currentSessionId);
  }

  // Determine playback behavior
  bool wasSameType = (currentState.currentTypeIndex == typeIndex);
  bool wasPlaying = currentState.isPlaying;
  bool isNewSession = false;

  if (wasSameType && wasPlaying) {
    // Mid-play same-type press: increment variant, interrupt current clip
    variantIndices[typeIndex] = getNextVariantIndex(typeIndex, modeName);
    request.action = PlaybackAction::START_CLIP;
  } else if (!wasSameType && wasPlaying) {
    // Different type during playback: switch type, reset to index 0
    resetVariantIndex(typeIndex);
    isNewSession = true;
    request.action = PlaybackAction::START_CLIP;
  } else {
    // Not playing or same type after completion: just start at current index
    isNewSession = true;
    request.action = PlaybackAction::START_CLIP;
  }

  // Ensure variant index is within bounds of the active variants for this mode
  auto activeVars = getActiveVariants(typeIndex, modeName);
  if (variantIndices[typeIndex] >= activeVars.size()) {
    variantIndices[typeIndex] = 0;
  }

  // Map the filtered index to the unfiltered index for the actual playback path
  uint8_t unfilteredIndex = mapFilteredToUnfilteredIndex(typeIndex, modeName, variantIndices[typeIndex]);

  // Create new session if needed
  if (isNewSession || currentSessionId == 0) {
    uint32_t timeSinceLastMs = 0;
    if (lastSessionEndTime > 0) {
      timeSinceLastMs = (eventTime - lastSessionEndTime) * 1000;
    }
    currentSessionId = logger->startSession(timeSinceLastMs);
  }

  // Update current state
  currentState.currentTypeIndex = typeIndex;
  currentState.currentVariantIndex = unfilteredIndex;
  currentState.isPlaying = true;
  currentState.playStartTimeMs = millis();
  currentState.lastInteractionTime = eventTime;
  currentState.currentModeIndex = modeResult.modeFound ? modeResult.modeIndex : 0;

  request.variantIndex = unfilteredIndex;

  // Get variant path for logging
  const char* variantPath = contentMgr->getVariantPath(typeIndex, request.variantIndex);
  if (!variantPath) variantPath = "unknown.wav";

  // TODO: Get expected duration from file metadata (for now, use 0)
  uint32_t expectedDurationMs = 0;

  logger->logPlaybackStarted(typeName, variantPath, modeName, expectedDurationMs, currentSessionId);

  return request;
}

void PlaybackControllerImpl::notifyClipFinished() {
  if (currentEasterEggPattern != EasterEggPattern::NONE) {
    uint32_t durationMs = millis() - easterEggStartTimeMs;
    uint8_t variantIndex = 0;
    if (eggState) {
      auto variants = contentMgr->getEasterEggVariants(currentEasterEggPattern);
      if (!variants.empty()) {
        uint8_t currentIndex = eggState->patternLoopIndex[(uint8_t)currentEasterEggPattern];
        variantIndex = (currentIndex + variants.size() - 1) % variants.size();
      }
    }
    const char* variantPath = contentMgr->getEasterEggVariantPath(currentEasterEggPattern, variantIndex);
    if (!variantPath) variantPath = "unknown.wav";
    const char* basename = strrchr(variantPath, '/');
    if (basename) basename++; else basename = variantPath;

    logger->logEasterEggEnded(currentEasterEggPattern, basename, durationMs, true, currentSessionId);
    currentEasterEggPattern = EasterEggPattern::NONE;
    return;
  }

  if (currentState.isPlaying && currentState.currentTypeIndex != 0xFF) {
    // Get names for logging
    const TypeContent* typeContent = contentMgr->getType(currentState.currentTypeIndex);
    const char* typeName = typeContent ? typeContent->folderName.c_str() : "unknown";
    const char* variantPath = contentMgr->getVariantPath(currentState.currentTypeIndex, currentState.currentVariantIndex);
    if (!variantPath) variantPath = "unknown.wav";
    const char* modeName = currentState.currentModeIndex < typeContent->modes.size()
      ? typeContent->modes[currentState.currentModeIndex].modeName.c_str()
      : "default";

    // Log raw completion
    logger->logPlaybackEnded(typeName, variantPath, modeName, 0, true, currentSessionId);

    // End session with intent analysis (expected duration = 0, actual = 0, completed fully)
    logger->endSession(currentSessionId, typeName, variantPath, 0, 0, true);

    // Reset index to 0 for next play
    resetVariantIndex(currentState.currentTypeIndex);

    currentState.isPlaying = false;
    currentState.currentTypeIndex = 0xFF;
    lastSessionEndTime = time(nullptr);
    currentSessionId = 0;
  }
}

const PlaybackState& PlaybackControllerImpl::getCurrentState() const {
  return currentState;
}

uint8_t PlaybackControllerImpl::getVariantIndex(uint8_t typeIndex) const {
  if (typeIndex < variantIndices.size()) {
    return variantIndices[typeIndex];
  }
  return 0;
}

void PlaybackControllerImpl::resetVariantIndex(uint8_t typeIndex) {
  if (typeIndex < variantIndices.size()) {
    variantIndices[typeIndex] = 0;
  }
}

void PlaybackControllerImpl::setVariantIndex(uint8_t typeIndex, uint8_t index) {
  if (typeIndex < variantIndices.size() && index < contentMgr->getVariantCount(typeIndex)) {
    variantIndices[typeIndex] = index;
  }
}

std::vector<std::string> PlaybackControllerImpl::getActiveVariants(uint8_t typeIndex, const char* modeName) {
  std::vector<std::string> result;
  const TypeContent* type = contentMgr->getType(typeIndex);
  if (!type) return result;

  std::string prefix = std::string(modeName) + "/";
  for (const auto& var : type->variants) {
    if (var.rfind(prefix, 0) == 0) { // starts with prefix
      result.push_back(var);
    }
  }

  // Fallback to "default/" if no variants found for this mode
  if (result.empty() && strcmp(modeName, "default") != 0) {
    std::string defaultPrefix = "default/";
    for (const auto& var : type->variants) {
      if (var.rfind(defaultPrefix, 0) == 0) {
        result.push_back(var);
      }
    }
  }

  return result;
}

uint8_t PlaybackControllerImpl::mapFilteredToUnfilteredIndex(uint8_t typeIndex, const char* modeName, uint8_t filteredIndex) {
  const TypeContent* type = contentMgr->getType(typeIndex);
  if (!type) return 0;

  auto activeVars = getActiveVariants(typeIndex, modeName);
  if (filteredIndex >= activeVars.size()) return 0;

  std::string targetVar = activeVars[filteredIndex];
  for (size_t i = 0; i < type->variants.size(); i++) {
    if (type->variants[i] == targetVar) {
      return i;
    }
  }
  return 0;
}

uint8_t PlaybackControllerImpl::getNextVariantIndex(uint8_t typeIndex, const char* modeName) {
  auto activeVars = getActiveVariants(typeIndex, modeName);
  uint8_t variantCount = activeVars.size();
  if (variantCount == 0) {
    return 0;
  }

  uint8_t current = variantIndices[typeIndex];
  uint8_t next = (current + 1) % variantCount;  // Wrap around
  return next;
}

const char* PlaybackControllerImpl::getLastError() const {
  return lastError;
}

void PlaybackControllerImpl::enterStandby() {
  inStandby = true;
  currentState.isPlaying = false;
  if (currentSessionId != 0) {
    logger->endSession(currentSessionId, nullptr, nullptr, 0, 0, false);
    currentSessionId = 0;
    lastSessionEndTime = time(nullptr);
  }
  logger->logStandbyEntered();
}

void PlaybackControllerImpl::exitStandby() {
  inStandby = false;
  logger->logStandbyExited();
}

bool PlaybackControllerImpl::isInStandby() const {
  return inStandby;
}

void PlaybackControllerImpl::setEasterEggState(EasterEggState* eggState) {
  this->eggState = eggState;
}

void PlaybackControllerImpl::handleEasterEggPattern(EasterEggPattern pattern) {
  if (pattern == EasterEggPattern::NONE) {
    return;
  }

  // If another easter egg was already playing, end it first
  if (currentEasterEggPattern != EasterEggPattern::NONE) {
    uint32_t durationMs = millis() - easterEggStartTimeMs;
    uint8_t prevIndex = 0;
    if (eggState) {
      auto variants = contentMgr->getEasterEggVariants(currentEasterEggPattern);
      if (!variants.empty()) {
        uint8_t currentIndex = eggState->patternLoopIndex[(uint8_t)currentEasterEggPattern];
        prevIndex = (currentIndex + variants.size() - 1) % variants.size();
      }
    }
    const char* prevPath = contentMgr->getEasterEggVariantPath(currentEasterEggPattern, prevIndex);
    if (!prevPath) prevPath = "unknown.wav";
    const char* prevBasename = strrchr(prevPath, '/');
    if (prevBasename) prevBasename++; else prevBasename = prevPath;

    logger->logEasterEggEnded(currentEasterEggPattern, prevBasename, durationMs, false, currentSessionId);
  }

  // If normal playback was active, log its interruption
  if (currentState.isPlaying && currentState.currentTypeIndex != 0xFF) {
    const TypeContent* typeContent = contentMgr->getType(currentState.currentTypeIndex);
    const char* typeName = typeContent ? typeContent->folderName.c_str() : "unknown";
    const char* variantPath = contentMgr->getVariantPath(currentState.currentTypeIndex, currentState.currentVariantIndex);
    if (!variantPath) variantPath = "unknown.wav";
    const char* modeName = currentState.currentModeIndex < typeContent->modes.size()
      ? typeContent->modes[currentState.currentModeIndex].modeName.c_str()
      : "default";

    logger->logPlaybackEnded(typeName, variantPath, modeName, millis() - currentState.playStartTimeMs, false, currentSessionId);
    
    currentState.isPlaying = false;
    currentState.currentTypeIndex = 0xFF;
  }

  // Check if easter egg folder has .wav files
  char eggFolderPath[128];
  snprintf(eggFolderPath, sizeof(eggFolderPath), "%s/%s", PATH_EASTER_EGG, getEasterEggPatternName(pattern));

  // Check if folder exists
  if (!SD.exists(eggFolderPath)) {
    return;  // No easter egg for this pattern
  }

  // Check if folder has any .wav files
  File eggFolder = SD.open(eggFolderPath);
  if (!eggFolder || !eggFolder.isDirectory()) {
    return;
  }

  bool hasWavFiles = false;
  File file;
  while (file = eggFolder.openNextFile()) {
    const char* fileName = file.name();
    int len = strlen(fileName);
    if (len > 4 && strcasecmp(fileName + len - 4, ".wav") == 0) {
      hasWavFiles = true;
      file.close();
      break;
    }
    file.close();
  }
  eggFolder.close();

  if (!hasWavFiles) {
    return;  // No .wav files in folder, skip easter egg
  }

  currentEasterEggPattern = pattern;
  easterEggStartTimeMs = millis();

  // Get variant index for this pattern
  uint8_t variantIndex = 0;
  if (eggState) {
    variantIndex = eggState->patternLoopIndex[(uint8_t)pattern];
    eggState->lastInteractionTime = millis();
  }

  // Get variant path from content manager
  const char* variantPath = contentMgr->getEasterEggVariantPath(pattern, variantIndex);
  if (!variantPath) {
    snprintf(lastError, sizeof(lastError), "No easter egg variants for pattern %u", (uint8_t)pattern);
    currentEasterEggPattern = EasterEggPattern::NONE;
    return;
  }

  // TODO: Request AudioPlayer to interrupt current playback and play easter egg variant
  // For now, just log it
  logger->logEasterEggTriggered(pattern, variantPath, currentSessionId);

  // Increment variant index for next trigger (with wrap-around)
  auto variants = contentMgr->getEasterEggVariants(pattern);
  if (!variants.empty() && eggState) {
    uint8_t nextIndex = (variantIndex + 1) % variants.size();
    eggState->patternLoopIndex[(uint8_t)pattern] = nextIndex;
  }
}

