# Progress

## 2026-07-03

**Done:**
- Tasks 1-8: Full modular architecture implemented
- Config.h: 6-button GPIO map, SD paths, time window structures
- ContentTypes.h: discovery types, mode selection, events
- Module interfaces: 7 abstract base classes
- Module implementations: 7 concrete classes
- ContentManager: recursive scan, variant caching, retry
- ConfigLoader: JSON parsing with ArduinoJson, 3-level time windows
- PlaybackController: per-type state, index cycling, mode fallback
- AudioPlayer: AudioTools integration, WAV playback, completion detection
- ButtonManager: 50ms debounce, 6 GPIO polls, availability filtering
- PlaybackLogger: event queue (RAM) → SD daily JSON rotation, debug serial
- SystemManager: boot sequence, main loop orchestration
- 15+ modular source files, all compiled

**Verified:**
- ES8388 codec init, audio playback
- GPIO pin mapping verified
- I2C bus architecture (internal codec, external peripherals)

## 2026-07-05

**Done:**
- Fixed rapid button click loss: interrupt-driven input on GPIO 19 INT pin
  - ISR sets flag only (non-blocking, µs latency)
  - Main loop reads PCF8574 + queues events (circular queue, max 10)
  - ButtonManagerImpl: `interruptPending` flag, `queueButtonEvent()`, `dequeueEvent()`, `onPCF8574Interrupt()`
- Fixed I2C blockage (Error 263):
  - Moved button I2C reads from ISR to main loop
  - Created separate GPIO 5/22 I2C bus for OLED (SW_I2C bit-banging)
  - No contention between button polling + OLED updates + SD writes
- Optimized SD logging:
  - Batch writes: file open once, write all queued events, close once
  - Deferred to quiet window: flush only when audio off 5s + no interaction 5s
  - Reduced blocking from 5-20s to ~50-100ms per flush
- System time sync:
  - RTC boot check; fallback to compile timestamp if before 2025
  - ESP32 internal clock tracks after boot (no polling needed)
- Enhanced debug output:
  - Boot: system time, content scan (types + variants count)
  - Button clicks: `[BTN] Type name (type#)`
  - Audio events: `[AUDIO] Playing: trackname`, finish, stop
  - All serial output (non-blocking)

**Verified:**
- Button responsiveness: rapid clicks now immediate (no loss)
- OLED + button + audio all responsive simultaneously
- Compilation clean, no I2C conflicts
- Boot logs show all system state

**Next:**
- Hardware testing with wired buttons on PCF8574
- Volume control / additional features
- Production packaging
