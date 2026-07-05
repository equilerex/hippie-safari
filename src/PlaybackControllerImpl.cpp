#include <Arduino.h>
#include <algorithm>
#include <cstring>
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
    variantIndices[typeIndex] = getNextVariantIndex(typeIndex);
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
  currentState.currentVariantIndex = variantIndices[typeIndex];
  currentState.isPlaying = true;
  currentState.playStartTimeMs = millis();
  currentState.lastInteractionTime = eventTime;
  currentState.currentModeIndex = modeResult.modeFound ? modeResult.modeIndex : 0;

  request.variantIndex = variantIndices[typeIndex];

  // Get variant path for logging
  const char* variantPath = contentMgr->getVariantPath(typeIndex, request.variantIndex);
  if (!variantPath) variantPath = "unknown.wav";

  // TODO: Get expected duration from file metadata (for now, use 0)
  uint32_t expectedDurationMs = 0;

  logger->logPlaybackStarted(typeName, variantPath, modeName, expectedDurationMs, currentSessionId);

  return request;
}

void PlaybackControllerImpl::notifyClipFinished() {
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

uint8_t PlaybackControllerImpl::getNextVariantIndex(uint8_t typeIndex) {
  uint8_t variantCount = contentMgr->getVariantCount(typeIndex);
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
