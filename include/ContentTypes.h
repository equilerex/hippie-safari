#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include "Config.h"

// ============================================================================
// CONTENT DISCOVERY & MANAGEMENT
// ============================================================================

// Result of scanning a single type folder
struct ContentScan {
  bool success = false;
  uint8_t typeIndex = 0xFF;
  std::string folderName;
  std::vector<std::string> variantPaths;  // Full paths to .wav files
  std::string errorMessage;
};

// State of SD card and content
struct ContentState {
  bool sdAvailable = false;
  bool contentDiscovered = false;
  uint8_t typesFound = 0;
  uint32_t lastScanTimeMs = 0;
  std::string lastError;
};

// ============================================================================
// MODE SELECTION RESULT
// ============================================================================

struct ModeSelectionResult {
  bool modeFound = false;
  uint8_t modeIndex = 0xFF;
  std::string modeName;
  int appliedPriority = -1;
  std::string reason;  // Why this mode was selected (or not)
};

// ============================================================================
// VARIANT INDEX PERSISTENCE
// ============================================================================

// Per-type variant index state (volatile RAM, not persisted across power loss)
struct VariantIndex {
  uint8_t typeIndex;
  uint8_t currentIndex;
  uint8_t totalVariants;
};

// ============================================================================
// BUTTON EVENT
// ============================================================================

struct ButtonEvent {
  uint8_t typeIndex;        // Which button was pressed
  uint32_t pressTimeMs;     // When it was pressed (from millis())
  bool isPress = true;      // true = press, false = release (for future hold/chord)
};

// ============================================================================
// PLAYBACK REQUEST
// ============================================================================

enum class PlaybackAction : uint8_t {
  START_CLIP = 0,           // Start playing a variant
  STOP_CLIP = 1,            // Stop current playback
  PAUSE_CLIP = 2,           // Pause (future)
  RESUME_CLIP = 3           // Resume (future)
};

struct PlaybackRequest {
  PlaybackAction action;
  uint8_t typeIndex;        // Type to play from
  uint8_t variantIndex;     // Variant within type
  uint32_t requestTimeMs;   // When requested
};

// ============================================================================
// AUDIO STREAM STATE
// ============================================================================

struct AudioStreamState {
  bool isOpen = false;
  bool isPlaying = false;
  uint32_t bytesRead = 0;
  uint32_t totalBytes = 0;
  std::string currentFilePath;
};

#endif // CONTENT_TYPES_H
