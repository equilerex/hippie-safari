# Hippy Safari — Implementation Progress

**Last updated:** 2026-07-03

---

## Session Summary
- Completed comprehensive implementation plan: `docs/2026-07-02-hippy-safari-implementation-plan.md`
- Updated plan with confirmed hardware details (AudioTools, ES8388, GPIO pins)
- Addressed 6 follow-up questions + critical review feedback:
  1. Mode selection on button press (not polling); RTC optional at boot
  2. Button → folder mapping (1:1 alphabetically, excess folders excluded)
  3. Power loss resets index to 0 (intentional); last click timestamp for recovery
  4. Write queue to avoid log spam; flush when playback inactive
  5. Serial debug logging (disabled by default, toggle for development)
  6. Easter egg via existing button detection (hold, chord); SimplifiedError handling (events only, no state snapshots)
  7. Progress tracking setup; critical review resolved

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
- [x] **Task 1: Core Data Structures** — Config.h (GPIO pins, paths, time windows, constants); ContentTypes.h (discovery, mode selection, events); module interfaces (ContentManager, ConfigLoader, PlaybackController, AudioPlayer, ButtonManager, PlaybackLogger, SystemManager)
- [ ] **Task 2: ContentManager** — SD discovery, recursive scan, variant caching
- [ ] **Task 3: ConfigLoader** — Mode-level JSON parsing, ArduinoJson integration

### State Management & Playback (Tasks 4–5)
- [ ] **Task 4: PlaybackController** — Per-type state, index cycling, mode fallback logic
- [ ] **Task 5: AudioPlayer** — AudioTools integration, WAV playback, on-complete callback

### Input & Logging (Tasks 6–7)
- [ ] **Task 6: ButtonManager** — FreeRTOS debounce, pattern detection (hold, chord), button→folder mapping
- [ ] **Task 7: PlaybackLogger** — Write queue, SD card rotating daily logs, flush on idle, serial debug logging

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
- Write queue: entries buffered in ESP32 RAM, flushed to SD when playback inactive
- Events: track completion, type switch, variant cycle, mode switch, recovery, button patterns (hold, chord), system events, errors
- Serial debug log: optional (disabled by default), toggle during development

**Button-to-Folder Mapping:**
- Available GPIO pins mapped 1:1 to type folders (alphabetically sorted)
- Excess types (more folders than pins) excluded from playback
- Mapping updated on content discovery (dynamic)

**Button Pattern Detection:**
- Debounce: FreeRTOS task (not ISR), 50ms stable window + tiny debounce on spam
- Hold: press > 2s = long press (Easter egg candidate)
- Chord: simultaneous buttons within ~100ms window (Easter egg candidate)
- Rapid-fire: click counting handled by PlaybackController index cycling

**Mode Selection (Time-Range Based):**
- Each mode has startTime/endTime (HH:MM format, wraps midnight)
  - Example: "friday" = 07:00 Fri → 06:00 Sat (all day Friday)
  - Default folder has no config (always active)
- RTC optional at boot; can sync later; logs editable when RTC available
- getModeForTime() called **only on button press** (not polling)
- Modes with no config or corrupt config ignored (fallback to default)
- If mode differs from previous press → index resets to 0 (per type)
- Logged as system event `mode_switch` on button press

---

## Execution Plan
Recommend subagent-driven development: one fresh subagent per task, review between tasks.
Or inline execution: batch tasks with checkpoints.
