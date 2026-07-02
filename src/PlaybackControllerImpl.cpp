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

  // Check if mode changed from previous press
  if (modeResult.modeFound && currentModeIndices[typeIndex] != modeResult.modeIndex) {
    // Mode switched: reset variant index
    resetVariantIndex(typeIndex);
    currentModeIndices[typeIndex] = modeResult.modeIndex;
    logger->logModeChanged(typeIndex, modeResult.modeIndex);
  }

  // Determine playback behavior
  bool wasSameType = (currentState.currentTypeIndex == typeIndex);
  bool wasPlaying = currentState.isPlaying;

  if (wasSameType && wasPlaying) {
    // Mid-play same-type press: increment variant, interrupt current clip
    variantIndices[typeIndex] = getNextVariantIndex(typeIndex);
    logger->logVariantChanged(typeIndex, variantIndices[typeIndex]);
    request.action = PlaybackAction::START_CLIP;
  } else if (!wasSameType && wasPlaying) {
    // Different type during playback: switch type, reset to index 0
    resetVariantIndex(typeIndex);
    request.action = PlaybackAction::START_CLIP;
  } else {
    // Not playing or same type after completion: just start at current index
    request.action = PlaybackAction::START_CLIP;
  }

  // Update current state
  currentState.currentTypeIndex = typeIndex;
  currentState.currentVariantIndex = variantIndices[typeIndex];
  currentState.isPlaying = true;
  currentState.playStartTimeMs = millis();
  currentState.lastInteractionTime = eventTime;
  currentState.currentModeIndex = modeResult.modeFound ? modeResult.modeIndex : 0;

  request.variantIndex = variantIndices[typeIndex];

  logger->logTypeSelected(typeIndex);
  logger->logPlaybackStarted(typeIndex, request.variantIndex);

  return request;
}

void PlaybackControllerImpl::notifyClipFinished() {
  if (currentState.isPlaying && currentState.currentTypeIndex != 0xFF) {
    logger->logPlaybackEnded(currentState.currentTypeIndex, currentState.currentVariantIndex);

    // Reset index to 0 for next play
    resetVariantIndex(currentState.currentTypeIndex);

    currentState.isPlaying = false;
    currentState.currentTypeIndex = 0xFF;
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
  logger->logStandbyEntered();
}

void PlaybackControllerImpl::exitStandby() {
  inStandby = false;
  logger->logStandbyExited();
}

bool PlaybackControllerImpl::isInStandby() const {
  return inStandby;
}
