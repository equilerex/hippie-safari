# Hippy Safari — Implementation Progress

**Last updated:** 2026-07-02

---

## Session Summary
- Completed comprehensive implementation plan: `docs/2026-07-02-hippy-safari-implementation-plan.md`
- Updated plan with confirmed hardware details (AudioTools, ES8388, GPIO pins)
- Addressed 6 follow-up questions:
  1. Mode prioritization logic (time-based, specificity ranking)
  2. Playback-during-mode-switch resilience (AudioPlayer::stop() on mode change)
  3. Button pattern detection (holds, chords, rapid-fire via FreeRTOS task debounce)
  4. Audio codec: AudioTools + AudioBoardStream(AudioKitEs8388V1) confirmed
  5. PlaybackLogger expanded: hold duration, multi-button chords, mode switches, audio latency, memory snapshots → SD card (not SPIFFS)
  6. Progress tracking setup

---

## Completed Tasks

### Planning Phase
- [x] **Technical Requirements** — Updated `docs/technical-project-descriptio.md` with playback behavior + logging sections
- [x] **Implementation Plan** — Created `docs/2026-07-02-hippy-safari-implementation-plan.md` with 10 tasks, component interfaces, graceful fallback matrix
- [x] **Hardware Confirmation** — Reviewed `docs/confirmed-setup.md` (AudioTools, ES8388, GPIO pins, SPI SD config)
- [x] **Plan Refinement** — Added mode prioritization, playback-during-switch handling, button patterns, SD logging strategy

---

## Pending Implementation Tasks

### Core Infrastructure (Tasks 1–3)
- [ ] **Task 1: Core Data Structures** — Types.h, Config.h (TypeContent, ModeContent, PlaybackState, ConfigData, LogEntry)
- [ ] **Task 2: ContentManager** — SD discovery, recursive scan, variant caching
- [ ] **Task 3: ConfigLoader** — Mode-level JSON parsing, ArduinoJson integration

### State Management & Playback (Tasks 4–5)
- [ ] **Task 4: PlaybackController** — Per-type state, index cycling, mode fallback logic
- [ ] **Task 5: AudioPlayer** — AudioTools integration, WAV playback, on-complete callback

### Input & Logging (Tasks 6–7)
- [ ] **Task 6: ButtonManager** — FreeRTOS debounce, pattern detection (hold, chord), type mapping
- [ ] **Task 7: PlaybackLogger** — SD card rotating logs, all event types (track completion, type switch, recovery, errors, memory)

### Integration & Testing (Tasks 8–10)
- [ ] **Task 8: main.cpp & System Integration** — Boot sequence, main loop, graceful recovery orchestration
- [ ] **Task 9: Integration Tests** — Full workflows (click → play → complete → log), fallback scenarios, multi-button patterns
- [ ] **Task 10: Documentation & Deployment** — Architecture diagrams, failure recovery tree, deployment checklist

---

## Notes for Next Session

**Confirmed Hardware:**
- Board: ESP32-A1S AudioKit v2.2
- Codec: ES8388 @ 0x10, AudioTools library v1.1.3
- Audio: AudioBoardStream(AudioKitEs8388V1), WAVDecoder, I2S pins confirmed
- Buttons: GPIO 5, 18, 19, 23 (active-low, INPUT_PULLUP)
- SD: FAT32, SPI pins 14 (SCK), 15 (MOSI), 2 (MISO), 13 (CS)
- Amp: GPIO 21, enabled LOW

**Logging Strategy:**
- Destination: SD card (`/logs/YYYY-MM-DD.json` rotating daily)
- Reason: SPIFFS is 1MB max, perf-limited; SD unlimited
- Events: track completion, type switch, variant cycle, mode switch, recovery, button patterns (hold, chord), system events, audio events, errors, memory snapshots

**Button Pattern Detection:**
- Debounce: FreeRTOS task (not ISR), 50ms stable window
- Hold: press > 2s = long press candidate (Easter egg)
- Chord: simultaneous buttons within 100ms window (Easter egg)
- Rapid-fire: click counting < 200ms already handled by PlaybackController index cycling

**Mode Selection (Time-Range Based):**
- Each mode has startTime/endTime (HH:MM format, wraps midnight)
  - Example: "friday" = 07:00 Fri → 06:00 Sat (all day Friday)
- RTC provides current time; ContentManager::getModeForTime() checks ranges
- Matching modes ranked by priority field (higher = preferred)
- Fallback to "default" if no time-range match
- PlaybackController polls every ~10s, detects transitions
- Mid-play mode switch: AudioPlayer::stop() → cleanly transitions
- Logged as system event `mode_selected` or `mode_switch`

---

## Execution Plan
Recommend subagent-driven development: one fresh subagent per task, review between tasks.
Or inline execution: batch tasks with checkpoints.
