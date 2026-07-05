# Easter Egg System Architecture

## Overview

Hidden pattern-triggered content system. Detects specific button interaction patterns and interrupts normal playback to play special easter egg audio variants. Fully content-driven — no firmware changes needed to add new patterns.

## Patterns (10 Total)

| Pattern | Trigger | Window | Detection |
|---------|---------|--------|-----------|
| SECRET_BUTTON | Press P7 (hidden GPIO) | — | Single press of special button |
| ASCENDING_SWEEP | Buttons 0→1→2→...→N in order | 2 sec | Sequential presses matching button count |
| SOS_MORSE | Tap pattern: 3 short, pause, 3 medium, pause, 3 short | 5 sec | Morse-code-like taps on any button |
| HAMMER_SINGLE | Same button 15+ times | 3 sec | Rapid repetition threshold |
| TEAM_EFFORT | All N buttons pressed simultaneously | 100ms | Chord detection within window |
| LONG_HOLD_SUSTAINED | Any button held 5+ sec | — | Sustained press duration |
| MULTI_HOLD | 2+ buttons held simultaneously | 3 sec | Multi-finger chord |
| ALL_BUTTONS_HELD | All N buttons held simultaneously | 5 sec | Full chord hold |
| CHAOS_BURST | 12+ random presses (mixed buttons) | 4 sec | Rapid mashing detection |
| MULTI_CLICK | 4+ presses of 2+ different buttons | 2 sec | Rapid alternation between buttons |

## Architecture Layers

### 1. Button Event Recording (ButtonManager)
- Records press/release events with timestamp
- Feeds to EasterEggDetector via `recordPress()` / `recordRelease()`
- Non-blocking — detector runs in main loop

### 2. Pattern Detection (EasterEggDetector)
- Maintains sliding event history (press/release events)
- `checkPattern()` runs after each button event
- Returns first matching pattern (priority-ordered)
- Stateless — no side effects, only detection
- Pattern-specific helpers check timing windows, button counts, sequences

### 3. Content Discovery (ContentManager)
- `discoverEasterEggs()` called at boot
- Scans `/audio/easter-egg/{PATTERN_NAME}/` folders
- Only triggers if folder contains `.wav` files
- Caches variant lists per pattern (sorted alphabetically)

### 4. Playback Interruption (PlaybackController)
- `handleEasterEggPattern(pattern)` called when pattern detected
- Checks SD for pattern folder + .wav files
- Increments variant loop index (wraps on end)
- Logs `EASTER_EGG_TRIGGERED` + `EASTER_EGG_ENDED` events
- Inherits session ID from interrupted normal playback

### 5. Variant Looping (EasterEggState)
- Per-pattern loop index stored in RAM (volatile)
- `patternLoopIndex[10]` array (one per pattern)
- Reset on 5-min silence timeout
- Persistent across repeated pattern triggers (same session)

## Data Flow

```
User presses buttons
  ↓
ButtonManager.recordPress() → EasterEggDetector
  ↓
EasterEggDetector.checkPattern() → returns EasterEggPattern
  ↓
PlaybackController.handleEasterEggPattern()
  ├─ Check /audio/easter-egg/{PATTERN_NAME}/ has .wav files
  ├─ Get current variant index from EasterEggState
  ├─ Load variant path from ContentManager
  ├─ Log EASTER_EGG_TRIGGERED
  ├─ [TODO] Interrupt current playback, request AudioPlayer
  └─ Increment loop index for next trigger
  ↓
[Playback ends]
  ↓
Log EASTER_EGG_ENDED with duration + completion status
```

## Folder Structure

```
/audio/easter-egg/
  SECRET_BUTTON/
    variant-01.wav
    variant-02.wav
  ASCENDING_SWEEP/
    ascending.wav
  SOS_MORSE/
    sos.wav
  HAMMER_SINGLE/
    woodpecker.wav
  TEAM_EFFORT/
    chorus.wav
  LONG_HOLD_SUSTAINED/
    meditation.wav
  MULTI_HOLD/
    duet.wav
  ALL_BUTTONS_HELD/
    crescendo.wav
  CHAOS_BURST/
    stampede.wav
  MULTI_CLICK/
    conversation.wav
  README.md
```

Each pattern folder can have 1-N `.wav` files. System cycles through them on repeated triggers.

## Wiring (SystemManager)

```cpp
// Boot sequence
easterEggDetector = new EasterEggDetectorImpl();
easterEggDetector->initialize(NUM_BUTTON_TYPES);
buttonMgr->setEasterEggDetector(easterEggDetector);

contentMgr->discoverEasterEggs();  // Scan folders

easterEggState = {0};  // Variant loop indices
playbackCtrl->setEasterEggState(&easterEggState);
```

## Logging

Easter egg events logged as:
- `EASTER_EGG_TRIGGERED` — pattern name, variant filename, session ID
- `EASTER_EGG_ENDED` — pattern name, duration played, completion status

Formatted as JSON in daily log files (`/logs/YYYY-MM-DD.json`).

## Future Work (Out of Scope)

- Load actual audio duration from file headers
- Interrupt + switch AudioPlayer to easter egg variant
- Dashboard visualization of easter egg events
- Admin tool to manage easter egg content (add/remove patterns)

## Design Notes

- Patterns detect on **first match** (priority-ordered for efficiency)
- Pattern windows are **hard-coded** (no configuration, prevents exploit loops)
- Variant loop **persistent in RAM** (lost on power cycle — intentional)
- Pattern folders **optional** — missing folder = pattern silently ignored
- No cross-pattern conflicts — each pattern has dedicated folder
- Loop reset timeout **5 minutes** of silence (configurable in Config.h)
