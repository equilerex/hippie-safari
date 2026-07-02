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
4. **Graceful Fallbacks:** Every failure point has recovery (missing file → next variant → silence; index out of bounds → reset to 0)
5. **Logging:** All user interactions + system events logged to SPIFFS for post-event analysis
6. **Library-First:** Use ArduinoJson, FreeRTOS, LittleFS; no custom JSON parsing, file systems, or debouncing

## Core Components

### 1. ContentManager
**Responsibility:** Discover and cache type/mode/variant structure from SD card.

**Interface concepts:**
- `discover()` → scans `/audio/types/` and `/audio/easter-egg/`, populates cache
- `getTypes()` → returns list of TypeContent (name, modes[], modes[].variants[])
- `getEasterEggs()` → returns easter egg types
- `hasMode(type, mode)` → fast query
- `getVariants(type, mode)` → list of variant filenames
- `getVariantPath(type, mode, index)` → full `/audio/types/...` path for variant (with bounds checking, fallback)

**Failure modes:** Missing folders → empty list; corrupt structure → skip and continue

### 2. ConfigLoader
**Responsibility:** Load and cache mode-level JSON configuration from SD card.

**Interface concepts:**
- `loadConfig(type, mode)` → reads `/audio/types/{type}/{mode}/config.json`
- Returns config object with loaded flag (true if file exists and valid)
- Generic key-value map (no hardcoded fields) for flexibility
- Falls back to empty config if file missing

**Failure modes:** Missing file → return empty config with `loaded=false`; invalid JSON → parse error, return empty config; continue normally

### 3. PlaybackController
**Responsibility:** State machine for button presses, index management per type, playback decisions.

**State tracked per type:**
- Current variant index (0-N)
- Last played mode
- Play count
- Last press timestamp

**Behaviors:**
- Button press → identify type → if same type as current, increment index (wrap); if different type, reset index to 0
- Index out of bounds → reset to 0
- Mode unavailable → fall back to `default` mode
- Variant unavailable at index → try next index; if none available, silence
- Track end → log completion, reset index to 0, return to idle

**Interface concepts:**
- `onButtonPress(buttonPin)` → ISR entry point
- `play(type, mode?)` → start playback at current index
- `stop()` → stop current playback, don't reset index
- `getCurrentState()` → returns {type, mode, index, isPlaying}

### 4. AudioPlayer
**Responsibility:** I2S codec management, audio file playback, interrupt handling.

**Use real library:** Audio codecs typically need drivers; ESP32-A1S boards have ES8388 codec with available libraries.

**Interface concepts:**
- `init()` → setup I2S, codec
- `playFile(filepath, onComplete=nullptr)` → start playback; callback fires when done
- `stop()` → stop immediately, pause codec
- `setVolume(0-255)` → adjust master volume
- `isPlaying()` → query current state

**Failure modes:** Missing file → callback fires with error; codec error → log, return to idle; I2S buffer underrun → log, graceful recovery

### 5. ButtonManager
**Responsibility:** Debounce GPIO button presses, map to type names, dispatch to PlaybackController.

**Use real library:** FreeRTOS task-based debouncing (not home-rolled delays).

**Interface concepts:**
- `init()` → register GPIO pins, setup ISR
- `setTypeMap(buttonPin → typeName)` → configure button-to-type mapping
- `onPress(callback)` → register callback for debounced press events

**Failure modes:** Button stuck → ISR spam ignored by debounce; GPIO read error → skip press

### 6. PlaybackLogger
**Responsibility:** Log all usage data to SPIFFS for post-event analysis.

**Interface concepts:**
- `logTrackPlayed(type, mode, variant)` → when track finishes
- `logTypeSwitch(fromType, toType)` → when user presses different button
- `logVariantCycle(type)` → when user presses same button again
- `logRecovery(event, details)` → graceful fallback (missing file, index reset, etc.)
- `logSystemEvent(event, details)` → boot, config load, etc.
- `logError(component, error, recovered)` → I2S error, JSON parse error, etc.
- `exportLogs()` → return JSON blob for retrieval

**Storage:** SPIFFS with rotating logs (current + archived)

**Failure modes:** SPIFFS full → drop oldest entries; write failure → skip log, continue; retrieval error → return empty logs

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
