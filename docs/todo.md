# Todo / Context Tracking

Context-only tracking list, not a spec or implementation plan. Captured during telemetry/analytics brainstorm session so these don't get lost.

## 1. Easter-egg index reset bug

The 5-minute analytics session-silence timeout currently also resets the real easter-egg pattern-detector state. These are two different concerns (analytics session boundary vs functional detector state) that got coupled — needs separating so egg detection isn't affected by the analytics timeout window.

## 2. Hardware expansion

- Wire a second PCF8574 expander (same type) to extend available button pins to 16 total.
- Add ~10 core buttons + 2-3 dedicated single-GPIO easter-egg buttons (directly wired, not pattern-detected).

## 3. Volume control

Add potentiometer-based volume control. Ties to volume-adjustment telemetry already noted in `docs/telemetryAndAnalytics.md`.

## 4. Blocking logic investigation (OLED vs buttons/audio)

Current documented constraint (`.ai/memory/decisions.md`, 2026-07-05): OLED kept static during playback specifically to avoid blocking the audio copy thread, since the OLED runs on bit-banged SW I2C which is CPU-blocking.

User believes there "must be" a way to decouple screen updates from button-listening/audio-playback rather than accepting this as permanent. Worth re-examining:
- Hardware I2C peripheral (vs bit-banged SW I2C) to remove the blocking cost.
- Splitting work across ESP32's second core via FreeRTOS task.
- Lower-frequency async/batched update scheme instead of per-frame blocking writes.

Not yet resolved — open investigation.

## 5. General cleanup pass

Mentioned but not yet scoped. Revisit this conversation for detail when picked up.
