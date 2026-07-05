#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <cstdint>
#include <ctime>
#include <vector>
#include <map>
#include <string>

// ============================================================================
// GPIO PIN MAPPING (ESP32-A1S AudioKit V2.2)
// ============================================================================

// Audio codec (ES8388 / I2S)
constexpr uint8_t PIN_I2S_BCLK = 27;
constexpr uint8_t PIN_I2S_LRCK = 26;
constexpr uint8_t PIN_I2S_DOUT = 35;
constexpr uint8_t PIN_I2S_DIN  = 25;
constexpr uint8_t PIN_I2S_MCLK = 0;

// I2C (codec config — internal, managed by audio lib)
constexpr uint8_t PIN_I2C_SDA = 33;
constexpr uint8_t PIN_I2C_SCL = 32;

// External I2C bus (RTC, OLED, PCF8574 — all share this bus)
constexpr uint8_t PIN_EXT_I2C_SDA = 18;
constexpr uint8_t PIN_EXT_I2C_SCL = 23;

// SD card (manual SPI control)
constexpr uint8_t PIN_SD_CS   = 13;
constexpr uint8_t PIN_SD_MISO = 2;
constexpr uint8_t PIN_SD_MOSI = 15;
constexpr uint8_t PIN_SD_SCK  = 14;

// Amplifier shutdown
constexpr uint8_t PIN_AMP_ENABLE = 21;
constexpr bool    AMP_ENABLED_STATE = LOW;

// User buttons — PCF8574 GPIO expander (addr 0x20)
// PCF8574 provides 8 ports (P0-P7), active-low (button pulls to GND)
// NUM_BUTTON_TYPES determined at runtime from content discovery
constexpr uint8_t MAX_BUTTON_TYPES = 7;  // P0-P6 (P7 reserved for easter egg)
extern uint8_t NUM_BUTTON_TYPES;         // Set at runtime after content discovery
extern uint8_t BUTTON_PINS[MAX_BUTTON_TYPES];  // Populated at runtime with ports for each type

// Hidden easter egg button — also on PCF8574
constexpr uint8_t PIN_EASTER_EGG = 7;    // PCF8574 port P7

// ============================================================================
// EASTER EGG PATTERNS
// ============================================================================

enum class EasterEggPattern : uint8_t {
  NONE = 0,
  SECRET_BUTTON = 1,           // Press hidden P7 button
  ASCENDING_SWEEP = 2,         // Press buttons 0→1→2→...→N in order
  SOS_MORSE = 3,               // Morse code: tap-tap-tap, pause, tap-hold-tap-hold-tap-hold, pause, tap-tap-tap
  HAMMER_SINGLE = 4,           // Same button 15+ times in 3 sec
  TEAM_EFFORT = 5,             // All N buttons pressed simultaneously (within 100ms)
  LONG_HOLD_SUSTAINED = 6,     // Hold any button 5+ sec
  MULTI_HOLD = 7,              // Hold 2+ buttons simultaneously for 3+ sec
  ALL_BUTTONS_HELD = 8,        // Hold all N buttons for 5+ sec
  CHAOS_BURST = 9,             // 12+ random button presses in 4 sec
  MULTI_CLICK = 10             // 4+ rapid presses of 2+ different buttons in 2 sec
};

// Easter egg timing windows (milliseconds)
constexpr uint32_t EASTER_EGG_WINDOW_CHORD_MS = 100;      // Buttons pressed within window for chord patterns
constexpr uint32_t EASTER_EGG_WINDOW_SEQUENCE_MS = 2000;  // Buttons pressed within window for sequence
constexpr uint32_t EASTER_EGG_WINDOW_RAPID_MS = 3000;     // Window for hammer/burst detection
constexpr uint32_t EASTER_EGG_WINDOW_CHAOS_MS = 4000;     // Window for chaos burst
constexpr uint32_t EASTER_EGG_HOLD_LONG_MS = 5000;        // Minimum hold duration for LONG_HOLD
constexpr uint32_t EASTER_EGG_HOLD_MULTI_MS = 3000;       // Minimum hold for MULTI_HOLD
constexpr uint32_t EASTER_EGG_HOLD_ALL_MS = 5000;         // Minimum hold for ALL_BUTTONS_HELD
constexpr uint32_t EASTER_EGG_LOOP_RESET_MS = 5 * 60 * 1000;  // 5 min silence = reset variant loop

// Easter egg pattern name strings
inline const char* getEasterEggPatternName(EasterEggPattern pattern) {
  switch (pattern) {
    case EasterEggPattern::SECRET_BUTTON: return "SECRET_BUTTON";
    case EasterEggPattern::ASCENDING_SWEEP: return "ASCENDING_SWEEP";
    case EasterEggPattern::SOS_MORSE: return "SOS_MORSE";
    case EasterEggPattern::HAMMER_SINGLE: return "HAMMER_SINGLE";
    case EasterEggPattern::TEAM_EFFORT: return "TEAM_EFFORT";
    case EasterEggPattern::LONG_HOLD_SUSTAINED: return "LONG_HOLD_SUSTAINED";
    case EasterEggPattern::MULTI_HOLD: return "MULTI_HOLD";
    case EasterEggPattern::ALL_BUTTONS_HELD: return "ALL_BUTTONS_HELD";
    case EasterEggPattern::CHAOS_BURST: return "CHAOS_BURST";
    case EasterEggPattern::MULTI_CLICK: return "MULTI_CLICK";
    default: return "UNKNOWN";
  }
}

// Onboard debug button (optional, direct GPIO — not part of PCF8574 array)
constexpr uint8_t PIN_BTN_DEBUG = 5;

// ============================================================================
// FILE PATHS & STORAGE
// ============================================================================

// Root audio path on SD
constexpr const char* PATH_AUDIO_ROOT = "/audio";
constexpr const char* PATH_TYPES_ROOT = "/audio/types";
constexpr const char* PATH_EASTER_EGG = "/audio/easter-egg";
constexpr const char* PATH_LOGS_ROOT  = "/logs";

// Config file for each type (relative to /audio/types/{type}/)
constexpr const char* CONFIG_FILENAME = "config.json";

// ============================================================================
// TIME WINDOWS & MODE CONFIG
// ============================================================================

enum class TimeWindowType {
  ABSOLUTE_EVENT,     // startDateTime + endDateTime (highest priority)
  DAY_SPECIFIC,       // days[] + startTime + endTime (medium priority)
  DAILY_RECURRING     // startTime + endTime (lowest priority)
};

struct TimeWindow {
  TimeWindowType type;

  // For ABSOLUTE_EVENT
  time_t absoluteStartTime = 0;
  time_t absoluteEndTime = 0;

  // For DAY_SPECIFIC and DAILY_RECURRING
  uint8_t startHour = 0;
  uint8_t startMin = 0;
  uint8_t endHour = 0;
  uint8_t endMin = 0;

  // For DAY_SPECIFIC only (bitmask: bit 0=Monday, ..., bit 6=Sunday)
  uint8_t dayMask = 0;

  int priority = 0;  // For tie-breaking when multiple windows match
};

struct ModeConfig {
  std::string modeName;
  std::vector<TimeWindow> timeWindows;  // Multiple windows with priorities
};

// ============================================================================
// CONTENT STRUCTURES
// ============================================================================

struct TypeContent {
  uint8_t typeIndex;              // 0..NUM_BUTTON_TYPES-1
  std::string folderName;         // e.g. "type_0_intro"
  std::vector<std::string> variants;  // List of .wav files discovered
  ModeConfig defaultMode;         // Always-active mode (no config needed)
  std::vector<ModeConfig> modes;  // Type-specific modes with time windows
};

// ============================================================================
// PLAYBACK STATE
// ============================================================================

struct PlaybackState {
  uint8_t currentTypeIndex = 0xFF;    // Current type (0xFF = none)
  uint8_t currentVariantIndex = 0;    // Variant index within type
  bool isPlaying = false;
  uint32_t playStartTimeMs = 0;       // When current clip started
  time_t lastInteractionTime = 0;     // Last button press timestamp
  uint8_t currentModeIndex = 0;       // Current active mode
};

// ============================================================================
// EVENT LOGGING
// ============================================================================

enum class EventType : uint8_t {
  TYPE_SELECTED = 0,
  VARIANT_CHANGED = 1,
  PLAYBACK_STARTED = 2,
  PLAYBACK_ENDED = 3,
  MODE_CHANGED = 4,
  SD_ERROR = 5,
  SD_RECOVERED = 6,
  CONFIG_ERROR = 7,
  STANDBY_ENTERED = 8,
  STANDBY_EXITED = 9,
  PLAYBACK_SESSION = 10,      // Summary event: one per clip completion with intent
  EASTER_EGG_TRIGGERED = 11,  // Easter egg pattern detected, starting playback
  EASTER_EGG_ENDED = 12       // Easter egg playback ended
};

enum class PlaybackIntent : uint8_t {
  UNKNOWN = 0,
  ENGAGED = 1,        // Listened to substantial portion
  EXPLORING = 2,      // Rapid cycling through variants/types
  REJECTED = 3,       // Interrupted early, switched type
  ERROR = 4           // Playback error or SD failure
};

struct PlaybackSessionContext {
  uint32_t pressCount;           // How many button presses to reach this clip
  uint8_t variantCycleCount;     // Rapid variant switches before settling
  uint32_t timeSinceLastMs;      // Gap from previous interaction
  bool modeChanged;              // Mode switch during session
  uint32_t interruptionDelayMs;  // If interrupted: when (0 if completed)
};

struct LogEntry {
  time_t timestamp;              // Unix timestamp
  EventType eventType;
  uint32_t sessionId;            // Session correlation ID
  PlaybackIntent intent;         // User intent (for PLAYBACK_SESSION events)
  const char* typeName;          // Content type name
  const char* variantName;       // Variant filename
  const char* modeName;          // Mode name
  uint32_t expectedDurationMs;   // Expected duration if playback started
  uint32_t actualDurationMs;     // Actual duration played
  bool completedFully;           // True if track played to end
  PlaybackSessionContext context; // Embedded session context (zeros if not PLAYBACK_SESSION)
  const char* details;           // Error/debug message
};

// ============================================================================
// EASTER EGG STATE
// ============================================================================

struct EasterEggState {
  uint8_t patternLoopIndex[10];  // Per-pattern variant index (indices 0-9 for patterns)
  uint32_t lastInteractionTime = 0;  // Last button press (any button) for timeout
};

// ============================================================================
// CONSTANTS
// ============================================================================

constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;
constexpr uint32_t SD_RETRY_INTERVAL_MS = 5000;  // Retry SD mount every 5s
constexpr uint32_t CONTENT_DISCOVERY_RETRY_MS = 10000;  // Retry content scan every 10s
constexpr uint32_t LED_BLINK_INTERVAL_MS = 500;  // LED blink when SD unavailable
constexpr size_t MAX_LOG_QUEUE_SIZE = 100;  // Max events in RAM before flush
constexpr size_t MAX_VARIANTS_PER_TYPE = 256;
constexpr size_t MAX_MODES_PER_TYPE = 10;
constexpr size_t MAX_TIME_WINDOWS_PER_MODE = 5;

// ============================================================================
// SYSTEM STATE
// ============================================================================

enum class SystemState : uint8_t {
  INITIALIZING = 0,
  READY = 1,
  STANDBY = 2,        // SD unavailable or no content
  PLAYING = 3,
  RECOVERING = 4      // Periodic retry in progress
};


#endif // CONFIG_H
