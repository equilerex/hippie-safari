# Known Failures

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
