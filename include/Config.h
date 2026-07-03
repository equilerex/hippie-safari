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

// I2C (codec config)
constexpr uint8_t PIN_I2C_SDA = 33;
constexpr uint8_t PIN_I2C_SCL = 32;

// SD card (manual SPI control)
constexpr uint8_t PIN_SD_CS   = 13;
constexpr uint8_t PIN_SD_MISO = 2;
constexpr uint8_t PIN_SD_MOSI = 15;
constexpr uint8_t PIN_SD_SCK  = 14;

// Amplifier shutdown
constexpr uint8_t PIN_AMP_ENABLE = 21;
constexpr bool    AMP_ENABLED_STATE = LOW;

// Button types (reliable pins only on ESP32-A1S AudioKit v2.2)
// GPIO 36: unreliable with Wi-Fi
// GPIO 13: disabled by SD card SPI
// GPIO 19: conflicts with onboard LED
// Working pins: 5, 18, 23 (3 buttons)
constexpr uint8_t PIN_BTN_TYPE0 = 5;   // Button 0 (Key 6)
constexpr uint8_t PIN_BTN_TYPE1 = 18;  // Button 1 (Key 5)
constexpr uint8_t PIN_BTN_TYPE2 = 23;  // Button 2 (Key 4)

// Number of button types enabled
constexpr uint8_t NUM_BUTTON_TYPES = 3;

// Array of button GPIO pins (in type order)
constexpr uint8_t BUTTON_PINS[NUM_BUTTON_TYPES] = {
  PIN_BTN_TYPE0, PIN_BTN_TYPE1, PIN_BTN_TYPE2
};

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
  STANDBY_EXITED = 9
};

struct LogEntry {
  time_t timestamp;           // Unix timestamp
  EventType eventType;
  uint8_t typeIndex;          // For relevant events
  uint8_t variantIndex;       // For variant-related events
  uint8_t modeIndex;          // For mode changes
  const char* details;        // Optional string (error message, etc.)
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
