# Easter Egg Pattern Detection & Playback

## Goal
Implement firmware-driven pattern detection that triggers special easter egg content when users interact with buttons in unintended ways (chords, sequences, rapid clicks, holds, chaos).

## Patterns (Hardcoded in Firmware)

All detection window times are measured from button state changes, tolerance ±50ms.

| Pattern | Trigger | Window | Example |
|---------|---------|--------|---------|
| `CHORD_2` | Any 2 buttons pressed | 100ms | Press buttons 0+1 simultaneously |
| `CHORD_3` | Any 3 buttons pressed | 100ms | Press buttons 0+1+2 simultaneously |
| `CHORD_ALL` | All buttons pressed | 100ms | Slam all 3 buttons at once |
| `SEQUENCE_FWD` | Buttons 0→1→2 in order | 2 sec total | Sequential left-to-right presses |
| `SEQUENCE_REV` | Buttons 2→1→0 in order | 2 sec total | Sequential right-to-left presses |
| `HAMMER_SINGLE` | Same button 5+ presses | 3 sec | Tap button 0 five times fast |
| `HAMMER_RAPID` | Any button 10+ presses | 5 sec | Mash any buttons (mixed), 10+ total |
| `HOLD_LONG` | Any button held | 5 sec continuous | Press and hold a button |
| `CHAOS` | 8+ presses (mixed buttons) | 4 sec | Random rapid mashing |

**Priority:** Patterns are checked in order above. First match wins. If no match, normal flow continues.

## Behavior

1. **Detection** → ButtonManager detects pattern, emits `PatternDetected(patternName)` event
2. **Intercept** → PlaybackController receives event, checks SD for `/audio/easter-egg/{PATTERN_NAME}/` folder
3. **Folder exists?**
   - YES: Interrupt current playback, load easter egg folder, play first variant (or next in loop)
   - NO: Ignore, continue normal flow
4. **Playback** → Easter egg inherits session ID from interrupted normal flow (logged as "parent session")
5. **Loop state** → Per-pattern variant index stored in RAM (resets after 5 min silence)
6. **Logging** → Log `EASTER_EGG_TRIGGERED` + `EASTER_EGG_ENDED` with pattern name, variant, duration

## Implementation Scope

### New Files

- `include/EasterEggDetector.h` — Pattern detection interface
- `src/EasterEggDetectorImpl.h` + `src/EasterEggDetectorImpl.cpp` — Pattern matching logic
- `include/EasterEggState.h` — Variant loop index persistence

### Modified Files

- `include/Config.h` — Add `enum EasterEggPattern`, timeout constants
- `include/PlaybackLogger.h` — Add `logEasterEggTriggered()`, `logEasterEggEnded()`
- `src/PlaybackLoggerImpl.h` + `.cpp` — Implement new logging methods
- `src/ButtonManagerImpl.*` — Emit `PatternDetected` event after press/release
- `src/PlaybackControllerImpl.cpp` — Hook easter egg check, interrupt on match
- `src/SystemManagerImpl.cpp` — Initialize EasterEggDetector, wire to ButtonManager
- `platformio.ini` — Add any new dependencies (none expected)

### Test Plan

1. Create test folders:
   - `/audio/easter-egg/HAMMER_SINGLE/test-hammer.wav`
   - `/audio/easter-egg/CHORD_ALL/team-effort.wav`
2. Rapid-tap button 0 five times → should interrupt normal flow, play test-hammer.wav
3. Press all buttons simultaneously → should interrupt, play team-effort.wav
4. Verify logs show `EASTER_EGG_TRIGGERED` with pattern name + variant filename
5. Wait 5+ min in silence → loop index should reset
6. Tap same pattern again → should restart from index 0

## Event Flow

```
User presses buttons
  ↓
ButtonManager debounces, detects pattern
  ↓
Emits PatternDetected(patternName)
  ↓
PlaybackController intercepts
  ↓
Checks /audio/easter-egg/{patternName}/ exists?
  ├─ NO → Continue normal flow
  └─ YES → 
      ├─ Log EASTER_EGG_TRIGGERED (pattern, variant, inherit sessionId)
      ├─ Interrupt current playback
      ├─ Load easter egg variants
      ├─ Get next variant from loop state
      ├─ Start playback
      ├─ On finish: Log EASTER_EGG_ENDED (duration, completed)
      └─ Return to normal flow
```

## Logging Extensions

```cpp
// Add to EventType enum
EASTER_EGG_TRIGGERED = 10,
EASTER_EGG_ENDED = 11,

// New log entry fields (add to LogEntry struct)
const char* patternName;  // e.g., "HAMMER_SINGLE"
uint8_t patternLoopIndex; // Which variant in easter-egg folder
```

Example log entries:
```json
{"ts":1783238850,"evt":10,"pattern":"HAMMER_SINGLE","var":"drum-solo-01.wav","sessionId":123,"inherited":true}
{"ts":1783238865,"evt":11,"pattern":"HAMMER_SINGLE","var":"drum-solo-01.wav","actDur":15000,"complete":true}
```

## Constraints

- Pattern detection happens in ButtonManager/Controller (low latency)
- No blocking SD operations during detection (check folder existence must be fast)
- Variant loop index is volatile RAM (lost on power cycle, intentional)
- Easter eggs take priority over normal flow (always interrupt)
- Pattern window times are hard (no configuration, prevents exploit loops)

## Open Questions

- Should rapid button releases reset pattern detection? (e.g., hold a button, release, press another = new pattern?)
  - **Decision:** Releases don't interrupt; only new presses count. Holding multiple buttons simultaneously triggers CHORD patterns.
- Should easter egg folders support subfolders per pattern, or flat naming?
  - **Decision:** Flat: `/audio/easter-egg/PATTERN_NAME/` folder contains variants, no subfolders.
- What's the timeout for loop index reset? (5 min? session end?)
  - **Decision:** 5 min of silence from last interaction (any button, normal or easter egg).

## Success Criteria

✓ Patterns detected reliably without false positives  
✓ Logs capture pattern name + variant for analytics  
✓ Easter egg interrupts normal flow seamlessly  
✓ Variant looping persists across repeated patterns  
✓ Dashboard displays easter egg events with pattern context  
✓ Button spam (40 presses/40 sec) doesn't create 40 log entries  
