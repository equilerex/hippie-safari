# Hippy Safari — Complete Implementation Plan

> **PLANNING MODE**: Architecture, design, task decomposition. Ready for actual coding.
> 
> **For developers:** Use superpowers:subagent-driven-development or superpowers:executing-plans to build task-by-task.

## Goal

Build content-driven interactive installation that discovers audio types/modes/variants from SD card, manages playback with per-type index persistence, handles all failures gracefully, and logs all usage statistics.

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
2. **Per-Type Index Persistence:** Each type maintains its own variant index (not per-mode)
3. **Mode-Level Config:** `config.json` optional at `/audio/types/{type}/{mode}/`
4. **Mode Prioritization:** Time-based modes (e.g., "friday", "night") ranked by specificity; RTC provides hour/day; precedence chain: requested → fallback list → default
5. **Graceful Fallbacks:** Every failure point has recovery (missing file → next variant → silence; index out of bounds → reset to 0)
6. **Playback Resilience:** Mid-play mode switches handled gracefully; AudioPlayer::stop() → next press uses new mode
7. **Logging to SD:** All interactions + system events logged to SD card (rotating daily); avoids SPIFFS capacity/perf limits
8. **Button Pattern Detection:** Hold detection (>2s), chord detection (simultaneous presses), rapid-fire (click counting); candidates for Easter eggs
9. **Library-First:** Use ArduinoTools (AudioBoardStream), ArduinoJson, FreeRTOS; no custom codec, JSON, or debouncing

## Mode Selection & Time-Based Ranges

**Problem:** Modes are time-range based (e.g., "friday" = 07:00 Fri → 06:00 Sat). Multiple modes may be active; need deterministic selection.

**Solution:**

1. Each mode defines time range in config (startTime, endTime in HH:MM format)
   - Example: `"friday": { "startTime": "07:00", "endTime": "06:00", "priority": 1 }`
   - Ranges wrap midnight (endTime < startTime = next-day end)
2. RTC provides current time (hour, minute, day of week)
3. ContentManager::getModeForTime(type) checks all modes in type's folder:
   - Match current time against all modes' ranges
   - Return highest-priority matching mode
   - Fallback to "default" if none match
4. Precedence: priority field (user-configured) > specificity (day-specific > generic)
5. Log mode selection: `logSystemEvent("mode_selected", {type, mode, reason})`

**Playback during mode switch:**
- RTC tick updates current time
- PlaybackController polls getModeForTime() at regular intervals (e.g., every 10s)
- If mode changes mid-play, call AudioPlayer::stop()
- Current track stops cleanly, no glitch
- Next button press uses new mode
- Logged as `logModeSwitch(oldMode, newMode, "time-range-transition")`

---

## Core Components

### 1. ContentManager
**Responsibility:** Discover and cache type/mode/variant structure from SD card; select mode based on time ranges.

**Interface concepts:**
- `discover()` → scans `/audio/types/` and `/audio/easter-egg/`, populates cache, parses mode time-ranges from config.json
- `getTypes()` → returns list of TypeContent (name, modes[], modes[].variants[], modes[].timeRange)
- `getEasterEggs()` → returns easter egg types
- `getModeForTime(type, currentTime)` → checks all modes' time ranges, returns best-fit mode (or "default")
- `hasMode(type, mode)` → fast query
- `getVariants(type, mode)` → list of variant filenames
- `getVariantPath(type, mode, index)` → full `/audio/types/...` path for variant (with bounds checking, fallback)

**Time-range parsing (from mode config.json):**
```json
{
  "startTime": "07:00",
  "endTime": "06:00",
  "priority": 1
}
```
- startTime, endTime in HH:MM (24-hour)
- Ranges wrap midnight if endTime < startTime
- Priority breaks ties (higher = preferred)

**Failure modes:** Missing folders → empty list; corrupt structure → skip and continue; invalid time range → use "default"

### 2. ConfigLoader
**Responsibility:** Load and cache mode-level JSON configuration from SD card, including time ranges.

**Interface concepts:**
- `loadConfig(type, mode)` → reads `/audio/types/{type}/{mode}/config.json`
- Returns config object with loaded flag (true if file exists and valid)
- Parses time-range fields: startTime (HH:MM), endTime (HH:MM), priority (int)
- Generic key-value map for other settings (volume, effects, etc.)
- Falls back to empty config if file missing

**Example config.json for mode "friday":**
```json
{
  "startTime": "07:00",
  "endTime": "06:00",
  "priority": 1,
  "volume": 0.8,
  "effects": "reverb"
}
```

**Failure modes:** Missing file → return empty config with `loaded=false`, time range assumes "default" (always active); invalid JSON → parse error, return empty config; continue normally

### 3. PlaybackController
**Responsibility:** State machine for button presses, index management per type, time-based mode selection, playback decisions.

**State tracked per type:**
- Current variant index (0-N)
- Current mode (selected by time range)
- Play count
- Last press timestamp
- Last mode check timestamp

**Behaviors:**
- Button press → identify type → check current time, select mode via ContentManager::getModeForTime()
- Same type as current → increment index (wrap); different type → reset index to 0
- Mode changed mid-play (time-range transition) → call AudioPlayer::stop(), log switch
- Index out of bounds → reset to 0
- Variant unavailable at index → try next index; if none available, silence
- Track end → log completion, reset index to 0, return to idle
- Periodic mode check (e.g., every 10s) → detect time-range transitions

**Interface concepts:**
- `onButtonPress(buttonPin)` → ISR entry point, triggers time-based mode check
- `play(type)` → query current time, select mode, start playback at current index
- `stop()` → stop current playback, don't reset index
- `checkModeChange()` → periodic poll, detect time-range transitions, stop playback if mode changed
- `getCurrentState()` → returns {type, mode, index, isPlaying}

### 4. AudioPlayer
**Responsibility:** I2S codec management, audio file playback, interrupt handling.

**Use real library:** AudioTools (`arduino-audio-tools v1.1.3`, `arduino-audio-driver v0.1.4`) with `AudioBoardStream(AudioKitEs8388V1)` + `WAVDecoder`.

**Hardware:** ESP32-A1S AudioKit v2.2, ES8388 codec @ 0x10, I2S pins confirmed, SPI SD pins confirmed.

**Interface concepts:**
- `init()` → setup I2S, codec via AudioBoardStream, enable amp (GPIO21 LOW)
- `playFile(filepath, onComplete=nullptr)` → open File from SD, pipe through EncodedAudioStream, callback fires when stream ends
- `stop()` → close file, end stream
- `setVolume(0-255)` → AudioBoardStream::setVolume()
- `isPlaying()` → query stream state
- `holdDuration()` → (optional) return ms since play start for button hold logic

**Failure modes:** Missing file → callback fires with error; codec error → log, return to idle; I2S buffer underrun → log, graceful recovery; SD read latency → log read time for perf analysis

### 5. ButtonManager
**Responsibility:** Debounce GPIO button presses, detect patterns (clicks, holds, chords), map to type names, dispatch to PlaybackController.

**Use real library:** FreeRTOS task-based debouncing (not ISR spin-loops); button pins active-low (HIGH=released, LOW=pressed).

**Button patterns detected:**
- **Click:** Press → release → within 50ms debounce window
- **Hold:** Pin LOW > 2s = long press (Easter egg trigger candidate)
- **Rapid-fire:** Multiple clicks < 200ms apart (already handled by PlaybackController index cycling)
- **Chord:** Multiple buttons pressed simultaneously within 100ms window (Easter egg candidate)

**Interface concepts:**
- `init()` → register GPIO pins (5, 18, 19, 23 confirmed), setup ISR to FreeRTOS task
- `setTypeMap(buttonPin → typeName)` → configure button-to-type mapping
- `onPress(button, holdMs, callback)` → register callback with hold duration for pattern detection
- `getChord()` → return set of currently pressed buttons (for simultaneous detection)

**Failure modes:** Button stuck → debounce ignores repeated fire; GPIO read error → skip press

### 6. PlaybackLogger
**Responsibility:** Log all usage data to SD card for post-event analysis.

**Events logged:**
- **Track played:** type, mode, variant, duration, completion flag
- **Type switch:** button press from type A → type B
- **Variant cycle:** same type pressed again (index increment)
- **Mode switch:** automatic (time-based) or manual override
- **Recovery events:** missing file, index reset, mode fallback, silent fallback
- **Button patterns:** hold duration, chord composition (multi-button), rapid-fire sequence
- **System events:** boot, SD mount, config load, RTC sync, content discovery
- **Audio events:** codec state, volume changes, audio latency metrics
- **Errors:** SD read failure, JSON parse error, I2S underrun (with recovery flag)
- **Memory snapshots:** heap free, SPIFFS usage (per N minutes)

**Interface concepts:**
- `logTrackPlayed(type, mode, variant, durationMs, completed)` → track completion
- `logTypeSwitch(fromType, toType)` → type change
- `logVariantCycle(type, fromIndex, toIndex)` → index increment
- `logModeSwitch(fromMode, toMode, reason)` → time-based or manual
- `logRecovery(event, details)` → fallback (missing file, index reset, etc.)
- `logButtonPattern(buttons[], holdMs, isChord)` → pattern detection
- `logSystemEvent(event, details)` → boot, config, discovery, RTC
- `logAudioEvent(codec, event, details)` → codec state, latency
- `logError(component, error, recovered)` → any failure with recovery flag
- `exportLogs()` → return JSON blob for retrieval

**Storage:** SD card (`/logs/YYYY-MM-DD.json` rotating by date; SPIFFS avoids for perf + capacity)

**Failure modes:** SD full → rotate out oldest day; write failure → skip log, continue; retrieval error → return empty logs

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

### Task 7: PlaybackLogger — Usage Statistics & Event Logging

**Scope:** Log all interactions and system events to SPIFFS JSON logs.

**Deliverables:**
- PlaybackLogger.h with logTrackPlayed, logTypeSwitch, logVariantCycle, logRecovery, logError, exportLogs
- Timestamp all entries (via RTC)
- Rotating logs (current + archived when size limit reached)
- Unit tests: log creation, export format, rotation behavior

**Verification:** Tests pass; logs contain all expected events with correct timestamps

---

### Task 8: main.cpp & System Integration

**Scope:** Initialize all subsystems, wire ISRs, implement main loop and graceful recovery.

**Deliverables:**
- main.cpp orchestrating ContentManager, ConfigLoader, PlaybackController, AudioPlayer, ButtonManager, PlaybackLogger
- Boot sequence: init hardware → discover content → setup buttons → ready to play
- Main loop: minimal (ISR-driven playback, log flushes, error recovery)
- Graceful recovery for all failure points (missing content, file errors, I2S errors)
- All subsystems initialize with fallback on error

**Verification:** System boots, plays audio on button press, logs events

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
- System architecture diagram (updated)
- Component interaction flowchart
- Failure recovery decision tree
- Deployment checklist (test audio files, verify GPIO pins, check SPIFFS space, etc.)
- Log analysis guide (how to interpret usage data)

**Verification:** Documentation complete, deployment checklist reviewed

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
- [ ] Use FreeRTOS for any timing-critical task (debounce, logging, audio)
- [ ] Const-correctness throughout (const refs, const methods)
- [ ] Smart pointers where heap allocation needed (unique_ptr, shared_ptr)
- [ ] RAII for all resources (file handles, I2S handles, mutexes)
- [ ] Logging everywhere failures can occur (not just on error)
- [ ] No blocking calls in ISR (ISR only sets flags, FreeRTOS task handles work)
- [ ] All variant/mode queries include bounds checking
- [ ] Every path returns graceful fallback, never abort()
- [ ] Audio paths use absolute paths; no relative path traversal
- [ ] Timestamps use RTC (prefer NTP if network available, but RTC is primary)

## Testing Strategy

1. **Unit tests** for each component (ContentManager, ConfigLoader, PlaybackController, etc.)
2. **Integration tests** for full workflows (button press → play → complete → log)
3. **Failure injection tests** (missing files, JSON corruption, I2S errors) 
4. **Real hardware tests** on actual ESP32-A1S board with real buttons and audio

---

## Open Questions / Future Scope

- Easter egg trigger detection (simultaneous button presses) — deferred, can be added post-launch
- Network time sync with RTC — optional enhancement for date-based variant selection
- Web UI for log retrieval and content management — future phase
- Real-time config reload (without reboot) — future phase
- Multi-language support — content-driven, not firmware (can add variant folders per language)

---

**Status:** Ready for implementation. Tasks are order-independent where possible; Task 1 (types) is prerequisite for all others.
