# Hippy Safari — Complete Implementation Plan

> **PLANNING MODE**: Architecture, design, task decomposition. Ready for actual coding.
> 
> **Baseline verified:** Button-triggered playback on ESP32-A1S already proven. This plan refactors from working baseline, preserving confirmed board/audio library setup.
> 
> **For developers:** Build task-by-task; focus on integrating verified playback with content discovery, variant cycling, mode selection, standby recovery, and logging.

## Goal

Integrate proven button-triggered audio playback with content-driven discovery (types/modes/variants from SD), per-type index persistence, folder-based button mapping, time-based mode selection, graceful failure handling, and event logging.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      HIPPY SAFARI SYSTEM                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────┐         ┌──────────────┐                     │
│  │   Buttons    │─────────│ ButtonManager│                     │
│  │ (6x GPIO)    │  ISR    │ (debounce,   │                     │
│  └──────────────┘         │ type map)    │                     │
│                           └──────┬───────┘                     │
│                                  │                             │
│                    ┌─────────────▼──────────────┐              │
│                    │    PlaybackController      │              │
│                    │ (state machine, index mgmt)│              │
│                    └──┬──────────────┬─────┬────┘              │
│                       │              │     │                  │
│         ┌─────────────┘   ┌──────────┘     │                  │
│         │                 │                │                  │
│    ┌────▼─────┐   ┌──────▼──────┐  ┌──────▼──────┐            │
│    │ Content  │   │  ConfigMgr  │  │ AudioPlayer │            │
│    │ Manager  │   │ (JSON parse)│  │ (I2S, codec)│            │
│    │ (SD scan)│   └─────────────┘  └─────────────┘            │
│    └────┬─────┘                                                │
│         │      ┌──────────────────────────────┐               │
│         └──────│     PlaybackLogger           │               │
│                │ (SPIFFS, JSON logging)       │               │
│                └──────────────────────────────┘               │
│                                                                 │
│  SD Card (/audio)          SPIFFS (/logs)                     │
│  types/ ───────────────    usage.json                         │
│  easter-egg/               events.json                        │
└─────────────────────────────────────────────────────────────────┘
```

## Key Design Decisions

1. **Content Discovery:** Recursive folder scan at boot; no hardcoded lists
2. **Button-to-Folder Mapping:** Available GPIO pins from a1s-board.md (2 reserved for SD card, optional i2s extension) mapped to content folders alphabetically; excess folders excluded
   - Folder prefix controls button order (e.g., `01_intro/`, `02_plain-clothes/`, `03_groups/`)
   - Changing numeric prefix changes button assignment without firmware changes
3. **Per-Type Index Persistence:** Each type maintains its own variant index (volatile RAM); power loss resets to 0 (intentional)
4. **Last Click Timestamp:** Stored per button press for recovery/debugging (runtime only, not required at boot); RTC optional, can sync later; timestamps can be delayed/edited when RTC available
5. **Mode Selection on Interaction:** getModeForTime() called only on button press (no polling); if mode differs from previous press, index resets to 0
6. **Mode Config Handling:** Default folder has no config (always active); missing/corrupt config in other modes → mode ignored (fallback to default)
7. **Responsive Playback:** Immediate play without delay, tiny debounce on button spam
   - Mid-play same-type press: interrupt, increment index, play new variant immediately
   - Different-type press during playback: interrupt, switch type, play at index 0
   - Track completes naturally: log completion, reset index to 0, return to standby
8. **Logging to SD:** Events logged to daily rotating file (`/logs/YYYY-MM-DD.json`); write-queued in memory to avoid rapid spam; flushed when playback inactive; stored on SD only
9. **Serial Debug Logging:** Separate debug log stream (disabled by default); enable for development/troubleshooting; outputs internal state changes; not written to SD
10. **Easter Eggs:** Content discovery supports `/audio/easter-egg/`; ButtonManager exposes generic events (click, hold, chord); final trigger routing remains open
11. **Library-First:** Use AudioTools (AudioBoardStream), ArduinoJson, FreeRTOS; preserve working playback baseline; avoid custom codecs/JSON/debounce unless library fails on hardware
12. **Audio Format:** WAV only; MP3 not supported
13. **Unavailable State:** If SD or content unavailable, enter standby with visible indicator (LED blink); periodically retry discovery; recover when available

## Mode Selection & Time-Based Ranges

**Problem:** Modes are time-range based (e.g., "friday" = 07:00 Fri → 06:00 Sat). Multiple modes may be active; need deterministic selection.

**Solution:**

Each mode config supports three time window models (prioritized):

1. **Absolute Event Window** (highest priority)
   - `startDateTime` + `endDateTime` (ISO 8601 or epoch timestamp)
   - One-time event window; overrides recurring patterns
   - Example: festival running Aug 15 14:00 → Aug 17 22:00

2. **Day-Specific Recurring Window**
   - `days` (array: ["monday", "friday"]) + `startTime` + `endTime` (HH:MM)
   - Repeats on specified days each week
   - Ranges wrap midnight if endTime < startTime
   - Example: `"days": ["friday"], "startTime": "07:00", "endTime": "06:00"` = Fri 07:00 → Sat 06:00

3. **Daily Recurring Window** (lowest priority)
   - `startTime` + `endTime` (HH:MM) without `days` array
   - Applies every day
   - Ranges wrap midnight if endTime < startTime
   - Example: `"startTime": "09:00", "endTime": "17:00"` = every day 09:00 → 17:00

**Config example:**
```json
{
  "startDateTime": "2026-08-15T14:00:00Z",
  "endDateTime": "2026-08-17T22:00:00Z",
  "priority": 100
}
```

**Mode selection logic:**

1. RTC provides current time (year, month, day, hour, minute) at runtime
   - RTC not required at boot; can be synced later
   - Logs can be delayed/edited when RTC available
2. PlaybackController::getModeForTime(type) called on user interaction (button press):
   - Check all modes' time windows against current RTC time
   - Return matching mode with highest priority (or "default" if none match)
   - If no config or corrupt config → mode ignored (fallback to next)
   - Priority ties resolve deterministically (e.g., by mode name alphabetically)
3. Mode switching on button press only (no polling)
   - User presses button → getModeForTime() called → select mode → play
   - If mode changed from previous press → index resets to 0 (per type)
   - Logged as `logModeSwitch(oldMode, newMode, "button-press")`
4. No RTC: all mode selections return "default" (system fully functional; time-based modes unavailable)

**Design note:** Type/button identity has no effect on index. Index increments per type, resets on mode switch or track completion.

---

## Core Components

### 1. ContentManager
**Responsibility:** Discover and cache type/mode/variant structure from SD card; provide mode selection based on time windows.

**Interface concepts:**
- `discover()` → scans `/audio/types/` and `/audio/easter-egg/`, populates cache, parses mode config.json files
- `getTypes()` → returns list of TypeContent sorted alphabetically (name, modes[], modes[].variants[], modes[].config)
- `getEasterEggs()` → returns easter egg types (from `/audio/easter-egg/`)
- `getModeForTime(type, currentTime)` → checks all modes' time windows, returns matching mode with highest priority (or "default" if none match; ignores modes with no config or corrupt config)
- `hasMode(type, mode)` → fast query
- `getVariants(type, mode)` → list of variant filenames (sorted alphabetically)
- `getVariantPath(type, mode, index)` → full `/audio/types/...` path for variant (with bounds checking; wraps on out-of-bounds)

**Time-window parsing (from mode config.json):**
```json
{
  "startDateTime": "2026-08-15T14:00:00Z",
  "endDateTime": "2026-08-17T22:00:00Z",
  "priority": 100
}
```
or
```json
{
  "days": ["friday"],
  "startTime": "07:00",
  "endTime": "06:00",
  "priority": 10
}
```
or
```json
{
  "startTime": "09:00",
  "endTime": "17:00",
  "priority": 5
}
```
- Priority breaks ties (higher = preferred); deterministic tie-break by mode name
- startTime/endTime in HH:MM (24-hour); ranges wrap midnight if endTime < startTime
- days array: ["monday", "tuesday", ..., "sunday"]; omit for daily recurrence
- startDateTime/endDateTime: ISO 8601 or epoch; overrides days + startTime/endTime

**Failure modes:** Missing folders → empty list; corrupt structure → skip and continue; invalid config → ignore mode (use "default")

### 2. ConfigLoader & RTC Support
**Responsibility:** Load and cache mode-level JSON configuration from SD card, including time ranges; graceful degradation without RTC.

**RTC Handling:**
- If RTC unavailable or not synced → getModeForTime() always returns "default" mode
- System functions normally; all types use "default" variant only
- Time-range modes (friday, night, etc.) inaccessible until RTC synced
- Once RTC synced, time-range modes become active on next button press

**Interface concepts:**
- `loadConfig(type, mode)` → reads `/audio/types/{type}/{mode}/config.json`
- Returns config object with loaded flag (true if file exists and valid)
- Parses time-range fields: startTime (HH:MM), endTime (HH:MM)
- Generic key-value map for other settings (volume, effects, etc.)
- Falls back to empty config if file missing

**Example config.json for mode "friday":**
```json
{
  "startTime": "07:00",
  "endTime": "06:00",
  "volume": 0.8,
  "effects": "reverb"
}
```

**Failure modes:** Missing file → return empty config with `loaded=false`; invalid JSON → parse error, return empty config; no RTC → all modes treated as "default"

### 3. PlaybackController
**Responsibility:** State machine for button presses, per-type index management, mode selection on interaction, playback decisions.

**State tracked per type:**
- Current variant index (0-N) in volatile RAM; reset to 0 on power loss (intentional)
- Current mode (selected by time range on button press)
- Last click timestamp (for recovery/debugging; runtime only)
- Last mode (detect mode change, reset index if different)

**Behaviors:**
- Button press → identify type → call getModeForTime() (check RTC time; returns "default" if no RTC) → select mode
- If mode differs from last mode for that type → reset index to 0
- Same type as previous press → increment index (wrap at variant_count); different type → index reset to 0 on mode selection
- Index out of bounds → reset to 0, try next variant until valid
- Variant missing at index → try next available variant (wrap); if all variants missing → silence
- **Track completes naturally** → log completion, reset index to 0, return to standby
- **Mid-play same-type press** → stop current, increment index, start new (immediate, no delay; tiny debounce on rapid spam)
- **Mid-play different-type press** → stop current, switch type, reset index to 0, start new variant at index 0
- No RTC → all mode selections return "default" (system fully functional, time-range modes unavailable)
- SD unavailable or no content discovered → enter standby with LED blink pattern; periodically retry discovery

**Interface concepts:**
- `onButtonPress(buttonPin)` → ISR entry point, routes to play(type)
- `play(type)` → query RTC time, select mode via getModeForTime(), get variant at current index, play immediately
- `stop()` → stop playback (called on button press during playback or mode switch)
- `getCurrentState()` → returns {type, mode, index, isPlaying}
- `getLastClickTime()` → returns timestamp of last press (for recovery)

### 4. AudioPlayer
**Responsibility:** I2S codec management, WAV audio file playback, interrupt handling.

**Use real library:** AudioTools (`arduino-audio-tools v1.1.3`, `arduino-audio-driver v0.1.4`) with `AudioBoardStream(AudioKitEs8388V1)` + `WAVDecoder`.

**Hardware:** ESP32-A1S AudioKit v2.2, ES8388 codec @ 0x10, I2S pins confirmed, SPI SD pins confirmed.

**Audio Format:** WAV files only (no MP3 support).

**Interface concepts:**
- `init()` → setup I2S, codec via AudioBoardStream, enable amp (GPIO21 LOW)
- `playFile(filepath, onComplete=nullptr)` → open WAV file from SD, pipe through DecoderStream, callback fires when stream ends
- `stop()` → close file, end stream
- `setVolume(0-255)` → AudioBoardStream::setVolume()
- `isPlaying()` → query stream state

**Failure modes:** Missing file → callback fires with error; codec error → log, return to idle; I2S buffer underrun → log, graceful recovery; invalid WAV format → treat as missing file

### 5. ButtonManager
**Responsibility:** Debounce GPIO button presses, detect patterns (holds, chords), map pins to types, dispatch to PlaybackController.

**Approach:** Prefer ESP32-compatible button library (e.g., `Button2`, `AceButton`) where practical. Avoid hand-rolled ISR logic unless selected library fails on hardware. Use FreeRTOS task-based event dispatch if needed; button pins active-low (HIGH=released, LOW=pressed).

**Button-to-Type Mapping:**
- Available GPIO pins from a1s-board.md (note: 2 pins reserved for SD card SPI)
- Content types (folders in /audio/types/) sorted alphabetically (folder prefix controls order; see Key Design Decisions #2)
- Map first N pins to first N types (1:1)
- Excess types (more folders than pins) excluded from playback
- Mapping updated on content discovery (dynamic)

**Button events exposed:**
- **Click:** Press → release → dispatch to PlaybackController (standard debounce; ~50ms stable window typical)
- **Hold:** Pin LOW > 2s = long press → dispatch hold event (Easter egg candidate)
- **Chord:** Multiple buttons pressed within 100ms window → dispatch chord event (Easter egg candidate)
- **Spam handling:** Tiny debounce on rapid repeats to prevent performance impact while user holds/spams

**Interface concepts:**
- `init()` → register available GPIO pins from a1s-board.md, initialize button library
- `buildTypeMap(types[])` → map sorted type names to available pins; return pin→type mapping
- `onButtonPress(callback)` → register callback with {pin, type, event, holdMs}
- `enablePatternDetection(bool)` → expose hold/chord events if library supports

**Failure modes:** Button stuck → debounce ignores repeated fire; GPIO read error → skip press

### 6. PlaybackLogger
**Responsibility:** Log all user interactions + errors to SD card for post-event analysis; queue writes in memory to avoid rapid writes.

**Events logged (interaction + error events only, no memory snapshots):**
- **Track played:** type, mode, variant, duration, completion flag
- **Type switch:** button press from type A → type B
- **Variant cycle:** same type pressed again (index increment)
- **Mode switch:** on button press (time-based mode change)
- **Recovery events:** missing file, variant fallback, silent fallback
- **Button patterns:** hold duration, chord composition (multi-button)
- **System events:** boot, SD mount, content discovery, RTC sync
- **Errors:** I2S error, SD read error, JSON parse error, codec error (with recovery flag if applicable; debounced to prevent log spam)

**Storage model:**
- Audio content: SD card (`/audio/types/`, `/audio/easter-egg/`)
- Usage logs: SD card (`/logs/YYYY-MM-DD.json`)
- Write queue: RAM (temporary buffer)
- No SPIFFS usage; all logging to SD

**Write strategy:**
- Log entries queued in memory (temporary ESP32 RAM buffer)
- Flushed to SD when playback inactive (best effort; can also flush on timer)
- Prevents rapid/repeated writes that degrade performance (e.g., don't log same error 10x/sec)
- Daily rotating file: `/logs/YYYY-MM-DD.json` (new file per day)

**Serial debug logging:**
- Separate debug log stream output to UART (disabled by default; enable for development)
- Outputs all internal state changes for troubleshooting (not written to SD or persisted)
- Can be toggled at runtime via `enableDebugLog(bool)`

**Interface concepts:**
- `logTrackPlayed(type, mode, variant, durationMs, completed)` → queued
- `logModeSwitch(fromMode, toMode)` → queued
- `logRecovery(event, details)` → queued
- `logButtonPattern(buttons[], holdMs, isChord)` → queued
- `logSystemEvent(event, details)` → queued
- `logError(component, error, recovered)` → queued (debounced to avoid spam; same error repeated rapidly = single log entry)
- `flushLogs()` → write queue to SD (called when playback inactive or on timer)
- `exportLogs()` → return JSON blob for retrieval
- `enableDebugLog(bool)` → toggle serial debug output

**Failure modes:** SD full → continue logging in memory; write failure → skip flush, continue queuing; retrieval error → return empty logs

## File Structure (To Be Created/Modified)

```
src/
  core/
    ContentManager.h/cpp
    ConfigLoader.h/cpp
    PlaybackController.h/cpp
    AudioPlayer.h/cpp
    ButtonManager.h/cpp
    PlaybackLogger.h/cpp
  
  hardware/
    A1SBoard.h/cpp          (existing; extend if needed)
    
  main.cpp                   (entry point; orchestrates all)

include/
  Config.h                   (constants, typedefs)
  Types.h                    (structs: TypeContent, ModeContent, PlaybackState, etc)

tests/
  test_content_manager.cpp
  test_config_loader.cpp
  test_playback_controller.cpp
  test_graceful_fallbacks.cpp
  test_playback_logger.cpp

docs/
  2026-07-02-hippy-safari-implementation-plan.md (this file)
```

## Task Breakdown (10 Implementation Tasks)

### Task 1: Define Core Data Structures & Types

**Scope:** Header files with all structs, enums, constants.

**Deliverables:**
- `include/Types.h` with TypeContent, ModeContent, PlaybackState, ConfigData, LogEntry structs
- `include/Config.h` with GPIO pins, paths, constants, button map
- All types use modern C++17 (structured bindings, optional, std::filesystem where applicable)

**Verification:** Types compile, no undefined symbols

---

### Task 2: ContentManager — SD Card Discovery & Caching

**Scope:** Scan `/audio/types/` and `/audio/easter-egg/` recursively, cache structure in RAM.

**Deliverables:**
- ContentManager.h with public interface (discover, getTypes, getVariants, getVariantPath)
- Efficient recursive scan; skip hidden files/folders
- Variant count per mode; graceful handling of empty modes
- Unit tests: scan with various folder structures, query methods, edge cases (missing folders, empty modes)

**Verification:** Tests pass; can query arbitrary type/mode/variant combinations

---

### Task 3: ConfigLoader — Mode-Level JSON Parsing

**Scope:** Load optional `/audio/types/{type}/{mode}/config.json`, parse with ArduinoJson.

**Deliverables:**
- ConfigLoader.h with loadConfig(type, mode) method
- Returns config struct with loaded flag
- Graceful fallback: missing file → empty config with loaded=false; invalid JSON → parse error log, empty config
- Unit tests: valid config, missing file, invalid JSON, edge cases

**Verification:** Tests pass; can load configs for any type/mode

---

### Task 4: PlaybackController — State Machine & Index Management

**Scope:** Track playback state per type, manage index cycling, handle mode/variant fallbacks.

**Deliverables:**
- PlaybackController.h with onButtonPress(pin) entry point
- Per-type state tracking (index, mode, play count, last timestamp)
- Button press → type ID → if same type, increment index (wrap); if different, reset index to 0
- Mode/variant availability checking with fallbacks
- Unit tests: button press sequences, index wrapping, mode fallback, variant fallback

**Verification:** Tests pass; all state transitions correct

---

### Task 5: AudioPlayer — I2S & Codec Management

**Scope:** Setup ESP32-A1S audio codec, handle file playback, interrupt handling.

**Deliverables:**
- AudioPlayer.h with init(), playFile(path, callback), stop(), setVolume(), isPlaying()
- Integrate ES8388 codec library (or equivalent for A1S board)
- WAV playback support (baseline); MP3 optional future work
- On-complete callback for track end detection
- Error recovery: file not found → callback with error; I2S error → log, graceful recovery

**Verification:** Can play audio files; callbacks fire on completion and error

---

### Task 6: ButtonManager — Debounce & Dispatch

**Scope:** Register GPIO buttons, debounce presses, map to type names, dispatch callbacks.

**Deliverables:**
- ButtonManager.h with init(), setTypeMap(), onPress(callback)
- FreeRTOS task-based debouncing (50ms stable window typical)
- Prevents ISR spam; accurate type identification
- Unit tests: button press sequences, debounce effectiveness, type map validation

**Verification:** Tests pass; ISR doesn't spam on stuck buttons

---

### Task 7: PlaybackLogger — Event Logging & SD Write Queue

**Scope:** Queue log entries in memory, flush to SD when idle; support daily rotating files and debug serial output.

**Deliverables:**
- PlaybackLogger.h with all log methods (logTrackPlayed, logTypeSwitch, logRecovery, logError, etc.)
- In-memory queue for log entries (temporary storage)
- flushLogs() writes queue to SD card (triggered on idle or timer)
- Daily rotating file: `/logs/YYYY-MM-DD.json`
- Optional serial debug log output (can be toggled)
- Timestamp entries (via RTC if available, or local millis if not)
- Unit tests: queue, flush, daily rotation, error debouncing

**Verification:** Tests pass; logs queued correctly, flush writes to SD, daily rotation works

---

### Task 8: main.cpp & System Integration

**Scope:** Initialize all subsystems, implement main loop, handle boot failures and standby/recovery states.

**Deliverables:**
- main.cpp orchestrating ContentManager, ConfigLoader, PlaybackController, AudioPlayer, ButtonManager, PlaybackLogger
- Boot sequence: init hardware → discover content → setup buttons → ready to play
- **Standby/recovery state:**
  - If SD card fails to mount or no content discovered: enter standby
  - Blink onboard LED in distinct pattern (e.g., 3 short blinks, 1s pause, repeat) to indicate system waiting
  - Periodically retry SD mount and content discovery (e.g., every 5s)
  - Recover automatically when SD/content becomes available
- Main loop: minimal (event-driven playback, log flushes when idle, periodic retry timer)
- Graceful recovery: missing file → next variant; variant unavailable → silence; I2S error → log + recover
- All subsystems initialize with fallback on error (except SD mount and content discovery; those trigger standby + retry)
- Use Arduino/ESP32 FS APIs for SD operations; preserve confirmed playback baseline

**Verification:** System boots to standby on missing SD/content, plays audio on button press once available, logs events, LED blinks to indicate standby state

---

### Task 9: Integration Tests — Full Workflows

**Scope:** End-to-end tests for common scenarios.

**Deliverables:**
- Test suite covering:
  - User presses type button → plays variant 0
  - User presses same button again → plays variant 1 (index increments)
  - User presses different button → switches type, resets index to 0
  - Missing variant → graceful fallback to next available
  - Index out of bounds → reset to 0
  - Missing mode → fallback to `default`
  - Track completes → index not reset (user can press again to cycle)
  - All events logged correctly

**Verification:** All workflows pass; logs match expected events

---

### Task 10: Documentation & Deployment Checklist

**Scope:** Final documentation, deployment guide, operational notes.

**Deliverables:**
- System architecture diagram (updated with all components)
- Component interaction flowchart (button press → mode selection → playback)
- Failure recovery decision tree (visual)
- Deployment checklist:
  - Audio files in correct structure (`/audio/types/{type}/{mode}/{variant}.wav`)
  - GPIO pins verified against actual board + pins mapped to type folders
  - SD card formatted FAT32, mounted correctly
  - RTC optional (can sync later); system works without it
  - Serial debug logging can be toggled during development
- Log analysis guide (structure, field meanings, how to interpret events)

**Verification:** Documentation complete, checklist reviewed

---

## Graceful Fallback Matrix

| Failure Scenario | Detection | Fallback | Recovery |
|---|---|---|---|
| Button GPIO read fails | ISR exception | Skip press | Log error, continue |
| Type not discovered | onButtonPress queries missing type | Use first available type | Log recovery event |
| Mode missing | ContentManager.hasMode() returns false | Use `default` mode | Log recovery, continue |
| Variant missing at index | getVariantPath() returns empty | Try next index; if all empty, silence | Log recovery, reset index |
| Config JSON invalid | ArduinoJson parse error | Use empty config | Log error, continue with defaults |
| Audio file missing | AudioPlayer cannot open | onComplete callback with error | Log error, stop, ready for next press |
| I2S error | Codec reports error | Stop playback, reset codec | Log error, graceful recovery |
| SPIFFS full | Write fails | Drop oldest log entries | Log truncation, continue |
| Index out of bounds | PlaybackController bounds check | Reset index to 0 | Log recovery, play variant 0 |
| Track completes | AudioPlayer callback | Log completion, reset index, idle | Ready for next press |

## Best Practices Checklist

- [ ] Use ArduinoJson for all JSON parsing (no manual string parsing)
- [ ] Use Arduino/ESP32 FS APIs for SD operations (not std::filesystem)
- [ ] Use FreeRTOS only for event dispatch; avoid blocking in ISR
- [ ] Library-first approach: prefer ESP32-compatible button library over custom ISR logic
- [ ] Const-correctness throughout (const refs, const methods)
- [ ] Smart pointers where heap allocation needed (unique_ptr, shared_ptr)
- [ ] RAII for all resources (file handles, I2S handles, mutexes)
- [ ] Logging everywhere failures can occur (not just on error; include info-level state transitions)
- [ ] No blocking calls in ISR (ISR only sets flags, FreeRTOS task handles work)
- [ ] All variant/mode queries include bounds checking
- [ ] Every path returns graceful fallback, never abort() or undefined behavior
- [ ] Audio paths use absolute paths; no relative path traversal
- [ ] Preserve confirmed working playback baseline (AudioTools, ES8388 codec setup)
- [ ] Error debouncing: don't log same error 10+ times/sec; queue and dedup in logger

## Testing Strategy

1. **Unit tests** for each component (ContentManager, ConfigLoader, PlaybackController, etc.)
2. **Integration tests** for full workflows (button press → play → complete → log)
3. **Failure injection tests** (missing files, JSON corruption, I2S errors) 
4. **Real hardware tests** on actual ESP32-A1S board with real buttons and audio

---

## Scope Boundaries (Out of Scope for This Phase)

The following are explicitly **not** part of this implementation:

- **Web UI / API** for log retrieval or content management
- **Live config reload** without reboot
- **Generic effects system** (reverb, EQ, etc.) — audio plays as-is
- **Alternative audio architecture** — preserve confirmed AudioTools baseline
- **Framework-style abstractions** — keep component split simple and direct
- **Easter egg routing finalization** — ButtonManager exposes events; final trigger mappings remain open

**In scope:**
- Content discovery and folder-based type mapping
- Per-type variant cycling with index persistence
- Mode selection based on time windows (with optional RTC)
- Event logging to SD
- Graceful failure recovery
- LED indicator for standby/error states
- Serial debug output

## Open Questions / Future Enhancements

- Easter egg trigger mappings (click + hold patterns, specific chords) — open after baseline stable
- Network time sync with NTP (separate from RTC) — optional enhancement
- Real-time config reload (without reboot) — future phase
- Multi-language support — content-driven, can add variant folders per language

---

**Status:** Ready for implementation. Tasks are order-independent where possible; Task 1 (types) is prerequisite for all others.
