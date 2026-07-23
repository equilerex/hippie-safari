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
constexpr uint8_t PIN_I2S_DIN = 25;
constexpr uint8_t PIN_I2S_MCLK = 0;

// I2C (codec config — internal, managed by audio lib)
constexpr uint8_t PIN_I2C_SDA = 33;
constexpr uint8_t PIN_I2C_SCL = 32;

// External I2C bus (RTC, OLED, PCF8574 — all share this bus)
constexpr uint8_t PIN_EXT_I2C_SDA = 18;
constexpr uint8_t PIN_EXT_I2C_SCL = 23;

// SD card (manual SPI control)
constexpr uint8_t PIN_SD_CS = 13;
constexpr uint8_t PIN_SD_MISO = 2;
constexpr uint8_t PIN_SD_MOSI = 15;
constexpr uint8_t PIN_SD_SCK = 14;

// Amplifier enable
constexpr uint8_t PIN_AMP_ENABLE = 21;
constexpr bool AMP_ENABLED_STATE = HIGH;

// User buttons — 3 identical PCF8574 GPIO expanders on the external I2C bus,
// distinguished by A0-A2 address jumpers. Each provides 8 ports (P0-P7),
// active-low (button pulls to GND). All three chips' open-drain INT pins are
// wired together onto the same GPIO19 line (see attachInterrupt in
// SystemManagerImpl.cpp) — any chip's changed input pulls the shared line low.
constexpr uint8_t PCF8574_ADDRESSES[] = {0x20, 0x24, 0x26};
constexpr uint8_t NUM_PCF8574_CHIPS = 3;
constexpr uint8_t PCF8574_PORTS = 8;

// Hidden easter egg button — physically chip 0 (addr 0x20), port P7
constexpr uint8_t EASTER_EGG_CHIP = 0;
constexpr uint8_t EASTER_EGG_PORT = 7;

// Logical button-type capacity: every port across all chips except the one
// physically reserved for the secret button above.
// NUM_BUTTON_TYPES determined at runtime from content discovery
constexpr uint8_t MAX_BUTTON_TYPES = (NUM_PCF8574_CHIPS * PCF8574_PORTS) - 1;  // 23
extern uint8_t NUM_BUTTON_TYPES;                    // Set at runtime after content discovery
extern uint8_t BUTTON_CHIP_INDEX[MAX_BUTTON_TYPES]; // Which PCF8574 chip (0..NUM_PCF8574_CHIPS-1) for type i
extern uint8_t BUTTON_PORT_INDEX[MAX_BUTTON_TYPES]; // Which port (0-7) on that chip for type i

// Logical button index fed to EasterEggDetector for the secret button.
// Deliberately outside the 0..MAX_BUTTON_TYPES-1 range used by real content
// buttons so it can never collide with (or be mistaken for) a real button's
// index — MAX_BUTTON_TYPES now spans multiple chips, so a small sentinel like
// the old value of 7 would otherwise land on a legitimate button.
constexpr uint8_t PIN_EASTER_EGG = 0xFF;

// ============================================================================
// EASTER EGG PATTERNS
// ============================================================================

enum class EasterEggPattern : uint8_t
{
  NONE = 0,
  SECRET_BUTTON = 1,       // Press hidden P7 button
  ASCENDING_SWEEP = 2,     // Press buttons 0→1→2→...→N in order
  SOS_MORSE = 3,           // Morse code: tap-tap-tap, pause, tap-hold-tap-hold-tap-hold, pause, tap-tap-tap
  HAMMER_SINGLE = 4,       // Same button 15+ times in 3 sec
  TEAM_EFFORT = 5,         // All N buttons pressed simultaneously (within 100ms)
  FAST_CLICK_PAIR = 6,     // 2 buttons pressed within 100ms
  LONG_HOLD_SUSTAINED = 7, // Hold any button 5+ sec
  MULTI_HOLD = 8,          // Hold 2+ buttons simultaneously for 3+ sec
  ALL_BUTTONS_HELD = 9,    // Hold all N buttons for 5+ sec
  CHAOS_BURST = 10,        // 12+ random button presses in 4 sec
  MULTI_CLICK = 11         // 4+ rapid presses of 2+ different buttons in 2 sec
};

// ============================================================================
// RUNTIME TUNING (timings for button + easter egg detection)
// ============================================================================
// All values below have compiled-in defaults, but can be overridden at boot
// by /safari-conf.json on the SD card root (see RuntimeTuning::loadFromSD).
// Lets timing be tuned on-device without a rebuild/reflash.
struct RuntimeTuning {
  // --- Button-level timings (ButtonManagerImpl) ---
  uint32_t buttonDebounceMs = 50;         // Min time between raw state changes to count as new
  uint32_t buttonClickMaxHoldMs = 1000;   // Release counts as a "click" only if held <= this

  // --- Easter egg: shared ---
  uint32_t eventHistoryWindowMs = 3000;   // Ring buffer retention window for tap-based patterns
  uint32_t patternCooldownMs = 1500;      // Min time between two egg triggers
  uint32_t maxTapHoldMs = 200;            // Tap-based eggs invalid if held longer than this
  uint32_t loopResetMs = 5 * 60 * 1000;   // Silence duration that resets per-pattern variant loop index

  // --- Easter egg: per-pattern ---
  uint32_t sweepWindowMs = 2000;          // ASCENDING_SWEEP: total time to complete sequence
  uint32_t sweepMinStepGapMs = 30;        // ASCENDING_SWEEP: min gap between steps (rules out chords)
  uint32_t sosWindowMs = 5000;            // SOS_MORSE: window to count taps in
  uint8_t  sosTapThreshold = 7;           // SOS_MORSE: taps required
  uint32_t hammerWindowMs = 3000;         // HAMMER_SINGLE: window
  uint8_t  hammerTapThreshold = 7;        // HAMMER_SINGLE: presses required
  uint32_t teamEffortWindowMs = 150;      // TEAM_EFFORT: max spread between all presses
  uint32_t fastClickPairWindowMs = 150;   // FAST_CLICK_PAIR: max spread between the 2 presses
  uint32_t longHoldMs = 3000;             // LONG_HOLD_SUSTAINED: min hold duration
  uint32_t multiHoldMs = 3000;            // MULTI_HOLD: min simultaneous hold duration
  uint32_t multiHoldJoinWindowMs = 1000;  // MULTI_HOLD: max spread between buttons joining the hold
  uint32_t allButtonsHeldMs = 5000;       // ALL_BUTTONS_HELD: min hold duration
  uint32_t allButtonsJoinWindowMs = 1000; // ALL_BUTTONS_HELD: max spread between buttons joining
  uint32_t chaosBurstWindowMs = 3000;     // CHAOS_BURST: window
  uint8_t  chaosBurstThreshold = 10;      // CHAOS_BURST: presses required
  uint32_t multiClickWindowMs = 2000;     // MULTI_CLICK: window
  uint8_t  multiClickThreshold = 4;       // MULTI_CLICK: presses required

  // Load overrides from /safari-conf.json on the SD root, if present.
  // Missing file or missing keys silently keep the compiled-in defaults.
  void loadFromSD();
};

// Global tuning instance — populated with defaults at startup, optionally
// overridden by RuntimeTuning::loadFromSD() during system init.
extern RuntimeTuning g_tuning;

// Easter egg pattern name strings
inline const char *getEasterEggPatternName(EasterEggPattern pattern)
{
  switch (pattern)
  {
  case EasterEggPattern::SECRET_BUTTON:
    return "SECRET_BUTTON";
  case EasterEggPattern::ASCENDING_SWEEP:
    return "ASCENDING_SWEEP";
  case EasterEggPattern::SOS_MORSE:
    return "SOS_MORSE";
  case EasterEggPattern::HAMMER_SINGLE:
    return "HAMMER_SINGLE";
  case EasterEggPattern::TEAM_EFFORT:
    return "TEAM_EFFORT";
  case EasterEggPattern::FAST_CLICK_PAIR:
    return "FAST_CLICK_PAIR";
  case EasterEggPattern::LONG_HOLD_SUSTAINED:
    return "LONG_HOLD_SUSTAINED";
  case EasterEggPattern::MULTI_HOLD:
    return "MULTI_HOLD";
  case EasterEggPattern::ALL_BUTTONS_HELD:
    return "ALL_BUTTONS_HELD";
  case EasterEggPattern::CHAOS_BURST:
    return "CHAOS_BURST";
  case EasterEggPattern::MULTI_CLICK:
    return "MULTI_CLICK";
  default:
    return "UNKNOWN";
  }
}

// Onboard debug button (optional, direct GPIO — not part of PCF8574 array)
constexpr uint8_t PIN_BTN_DEBUG = 5;

// Parse "YYYY-MM-DDTHH:MM:SSZ" into Unix epoch (UTC). Returns 0 on parse failure.
inline time_t parseIso8601Utc(const char *str)
{
  int y, mo, d, h, mi, sec;
  if (sscanf(str, "%d-%d-%dT%d:%d:%dZ", &y, &mo, &d, &h, &mi, &sec) != 6)
  {
    return 0;
  }
  // Days-since-epoch via civil_from_days (Howard Hinnant algorithm) - avoids
  // timegm()/mktime() dependence on libc TZ handling.
  int a = (14 - mo) / 12;
  int yy = y + 4800 - a;
  int mm = mo + 12 * a - 3;
  long jdn = d + (153 * mm + 2) / 5 + 365L * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
  const long jdnEpoch = 2440588; // Julian day number for 1970-01-01
  long days = jdn - jdnEpoch;
  return (time_t)(days * 86400L + h * 3600 + mi * 60 + sec);
}

// ============================================================================
// FILE PATHS & STORAGE
// ============================================================================

// Root audio path on SD
constexpr const char *PATH_AUDIO_ROOT = "/audio";
constexpr const char *PATH_TYPES_ROOT = "/audio/types";
constexpr const char *PATH_EASTER_EGG = "/audio/easter-egg";
constexpr const char *PATH_LOGS_ROOT = "/logs";

// Config file for each type (relative to /audio/types/{type}/)
constexpr const char *CONFIG_FILENAME = "config.json";

// ============================================================================
// TIME WINDOWS & MODE CONFIG
// ============================================================================

enum class TimeWindowType
{
  ABSOLUTE_EVENT, // startDateTime + endDateTime (highest priority)
  DAY_SPECIFIC,   // days[] + startTime + endTime (medium priority)
  DAILY_RECURRING // startTime + endTime (lowest priority)
};

struct TimeWindow
{
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

  int priority = 0; // For tie-breaking when multiple windows match
};

struct ModeConfig
{
  std::string modeName;
  std::vector<TimeWindow> timeWindows; // Multiple windows with priorities
};

// ============================================================================
// CONTENT STRUCTURES
// ============================================================================

struct TypeContent
{
  uint8_t typeIndex;                 // 0..NUM_BUTTON_TYPES-1
  std::string folderName;            // e.g. "type_0_intro"
  std::vector<std::string> variants; // List of .wav files discovered
  ModeConfig defaultMode;            // Always-active mode (no config needed)
  std::vector<ModeConfig> modes;     // Type-specific modes with time windows
};

// ============================================================================
// PLAYBACK STATE
// ============================================================================

struct PlaybackState
{
  uint8_t currentTypeIndex = 0xFF; // Current type (0xFF = none)
  uint8_t currentVariantIndex = 0; // Variant index within type
  bool isPlaying = false;
  uint32_t playStartTimeMs = 0;   // When current clip started
  time_t lastInteractionTime = 0; // Last button press timestamp
  uint8_t currentModeIndex = 0;   // Current active mode
};

// ============================================================================
// EVENT LOGGING
// ============================================================================

enum class EventType : uint8_t
{
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
  PLAYBACK_SESSION = 10,     // Summary event: one per clip completion with intent
  EASTER_EGG_TRIGGERED = 11, // Easter egg pattern detected, starting playback
  EASTER_EGG_ENDED = 12      // Easter egg playback ended
};

enum class PlaybackIntent : uint8_t
{
  UNKNOWN = 0,
  ENGAGED = 1,   // Listened to substantial portion
  EXPLORING = 2, // Rapid cycling through variants/types
  REJECTED = 3,  // Interrupted early, switched type
  ERROR = 4      // Playback error or SD failure
};

struct PlaybackSessionContext
{
  uint32_t pressCount;          // How many button presses to reach this clip
  uint8_t variantCycleCount;    // Rapid variant switches before settling
  uint32_t timeSinceLastMs;     // Gap from previous interaction
  bool modeChanged;             // Mode switch during session
  uint32_t interruptionDelayMs; // If interrupted: when (0 if completed)
};

struct LogEntry
{
  time_t timestamp; // Unix timestamp
  EventType eventType;
  uint32_t sessionId;             // Session correlation ID
  PlaybackIntent intent;          // User intent (for PLAYBACK_SESSION events)
  const char *typeName;           // Content type name
  const char *variantName;        // Variant filename
  const char *modeName;           // Mode name
  uint32_t expectedDurationMs;    // Expected duration if playback started
  uint32_t actualDurationMs;      // Actual duration played
  bool completedFully;            // True if track played to end
  PlaybackSessionContext context; // Embedded session context (zeros if not PLAYBACK_SESSION)
  const char *details;            // Error/debug message
};

// ============================================================================
// EASTER EGG STATE
// ============================================================================

struct EasterEggState
{
  uint8_t patternLoopIndex[10];     // Per-pattern variant index (indices 0-9 for patterns)
  uint32_t lastInteractionTime = 0; // Last button press (any button) for timeout
};

// ============================================================================
// CONSTANTS
// ============================================================================

constexpr uint32_t SD_RETRY_INTERVAL_MS = 5000;        // Retry SD mount every 5s
constexpr uint32_t CONTENT_DISCOVERY_RETRY_MS = 10000; // Retry content scan every 10s
constexpr uint32_t LED_BLINK_INTERVAL_MS = 500;        // LED blink when SD unavailable
constexpr size_t MAX_LOG_QUEUE_SIZE = 100;             // Max events in RAM before flush
constexpr size_t MAX_VARIANTS_PER_TYPE = 256;
constexpr size_t MAX_MODES_PER_TYPE = 10;
constexpr size_t MAX_TIME_WINDOWS_PER_MODE = 5;

// ============================================================================
// SYSTEM STATE
// ============================================================================

enum class SystemState : uint8_t
{
  INITIALIZING = 0,
  READY = 1,
  STANDBY = 2, // SD unavailable or no content
  PLAYING = 3,
  RECOVERING = 4 // Periodic retry in progress
};

#endif // CONFIG_H
