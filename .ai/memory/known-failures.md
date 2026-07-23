# Known Failures

## 0. Buttons beyond the 3rd silently dead + secret-button could corrupt stack memory

### Symptom
- With more than 3 content folders on the SD card, boot log always showed `Types discovered: 3` regardless of how many folders actually existed, and physical buttons wired past the first 3 PCF8574 ports produced no `[BTN-EVT]` log lines at all — not even debounce noise. Reported as "some buttons do not work at all."
- Separately, pressing the hidden secret button (P7) while 2+ other buttons were held could corrupt adjacent stack memory in rare cases — reported as "some buttons crash the whole system."

### Cause
1. `NUM_BUTTON_TYPES` (`src/SystemManagerImpl.cpp`) was hardcoded to `3` and never reassigned, despite its own comment claiming "Set at runtime after content discovery." `ContentManagerImpl::discoverContent()` uses that same variable as its scan cap, so folders beyond the 3rd were never even scanned, and `ButtonManagerImpl::initialize()` only ever set up `buttons[0..2]`.
2. `EasterEggDetectorImpl::hasInterferingEvent(const bool* exceptButtons, ...)` indexed `exceptButtons[event.buttonIndex]` with no bounds check. The secret button's logical index (`PIN_EASTER_EGG`, formerly `7`) is deliberately `>= numButtons` (that's how `isValidButton()` distinguishes it from a real content button), so any secret-button event scanned by `detectMultiHold()`'s call into this function read one byte past the end of a stack-allocated `bool[MAX_BUTTON_TYPES]` array — undefined behavior, worse once `MAX_BUTTON_TYPES` grew for multi-chip support (see 2026-07-23 decision on multi-chip PCF8574 button expansion in `decisions.md`).

### Fix
1. Default `NUM_BUTTON_TYPES = MAX_BUTTON_TYPES` before content discovery runs, then narrow it to `contentMgr->getTypeCount()` immediately after discovery completes and before `ButtonManagerImpl::initialize()` runs (`SystemManagerImpl.cpp`).
2. Add `if (event.buttonIndex >= numButtons) continue;` in `hasInterferingEvent(const bool*, ...)` before indexing `exceptButtons[]` (`EasterEggDetectorImpl.cpp`).

### Verification
Build, flash, and watch the serial console. Boot log should show `[BOOT] Content scan: N types` matching the actual number of folders under `/audio/types` (up to `MAX_BUTTON_TYPES`), and `Types discovered: N` at the end of init — not stuck at 3. Confirmed live on hardware via COM3 with 10 folders present, reporting `Content scan: 10 types` and `Types discovered: 10` after the fix (previously `3`).

---

## 1. External I2C Bus Timeout (Error 263 / ESP_ERR_TIMEOUT)

### Symptom
On boot, the system console logs `i2cRead returned Error 263` and prints `[DEBUG] ButtonManager: PCF8574 not responding — buttons disabled`. As a result, no buttons register actions, despite the physical hardware working perfectly prior to software changes.

### Cause
1. **Third-Party Library Bus Reset**: Calling `rtc.begin(&extI2C)` (Adafruit `RTClib`) and `audioKit->begin(cfg)` (Phil Schatzmann's `arduino-audio-driver` / `arduino-audio-tools` for ES8388 codec) re-initializes the microcontroller's I2C hardware drivers. By default, their internal `.begin()` methods re-run I2C bus initialization without pin arguments, resetting custom `TwoWire` instances (like `extI2C` / `Wire1`) to the default hardware pins of the microcontroller (away from custom pins GPIO 18/SDA and GPIO 23/SCL).
2. **PCF8574 Quasi-bidirectional Port Latches**: Replacing the library's `pcf8574->begin()` initialization call with raw I2C `requestFrom` reads bypassed the setup of the PCF8574 port latch register. On soft resets (RTS pin reset without cycling power to the PCF8574 chip), the I/O expander pins retain their previous latch values. If they were previously set to `LOW`, they act as outputs sinking current to GND, which prevents reading the pins as inputs and can trigger bus errors or locks.

### Fix
1. **Re-establish Custom Pin Routing**: Call `extI2C.begin(PIN_EXT_I2C_SDA, PIN_EXT_I2C_SCL)` in `SystemManagerImpl.cpp` immediately after the RTC initialization and immediately before `ButtonManager` initialization to override library resets.
2. **Use Library PCF8574 Object**: Re-enable `pcf8574->begin()` (which checks connection and writes `0xFF` to all ports to establish them as quasi-bidirectional inputs) and use `pcf8574->read8()` for single-transaction 8-bit port reads.

### Verification
Build, flash, and watch the serial console on COM3. The console should log:
```
[DEBUG] ButtonManager: Initializing PCF8574...
[DEBUG] ButtonManager: PCF8574 initialized
[DEBUG] ButtonManager: Port 0 initialized, initial state: 1
...
[DEBUG] ButtonManager initialized successfully
```

---

## 2. Easter Egg Detection Non-functional

### Symptom
Button events were recorded in `EasterEggDetector` but easter egg audio clips never triggered or logged.

### Cause
The method `checkEasterEggPattern()` was implemented in `ButtonManagerImpl` but was never called in the main loop or the handle event loop of `SystemManagerImpl.cpp`. Consequently, the state of the detector was never polled, preventing any pattern from being resolved, logged, or played. Additionally, the playback controller only contained a `TODO` for interrupting normal playback to play easter egg variants.

### Fix
1. **Poll Detector on Every Tick**: Call `buttonMgr->checkEasterEggPattern()` in `SystemManagerImpl::update()`. This runs in microseconds and catches both sequential mashing patterns and duration-dependent hold patterns.
2. **Implement Facade Playback Coordination**: If an easter egg pattern is matched in `SystemManagerImpl::update()`, stop any active normal audio, read the variant file path using the pattern's volatile loop index, play the file via `AudioPlayer`, and update the display and Playback Controller state.
3. **Log Interruptions & Natural Completion**: Update `PlaybackControllerImpl` to log `PLAYBACK_ENDED` (interrupted) when normal playback is interrupted by an easter egg, and log `EASTER_EGG_ENDED` (with `completedFully` status) when an easter egg is interrupted by a normal button press or finishes playing.
