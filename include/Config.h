#ifndef CONFIG_H
#define CONFIG_H


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

// User buttons — now on PCF8574 GPIO expander (addr 0x20)
// PCF8574 provides 8 ports (P0-P7), active-low (button pulls to GND)
// Port indices for each content type (adjusted per content discovery)
constexpr uint8_t NUM_BUTTON_TYPES = 3;  // 3 content types; adjust if more/fewer types discovered
constexpr uint8_t BUTTON_PINS[NUM_BUTTON_TYPES] = {
  0, 1, 2  // PCF8574 ports P0, P1, P2 (was ESP32 GPIOs 5, 18, 23)
};

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
  PLAYBACK_SESSION = 10  // Summary event: one per clip completion with intent
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
