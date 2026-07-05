# Decisions

### 2026-07-03: Separate I2C buses for audio and peripherals

**Decision:**
- Internal I2C (GPIO32/33): ES8388 codec only
- External I2C (GPIO18/23): OLED, RTC, PCF8574, future sensors

**Reason:**
- Audio library manages internal bus; prevents coupling external hardware to audio subsystem
- Verified to work without conflicts

**Rejected:**
- Single shared I2C bus (would tightly couple audio and peripherals)

**Impact:**
- All new I2C devices must use GPIO18/23 bus via `TwoWire(1)` instance
- Do not attach anything to GPIO32/33

---

### 2026-07-03: Mode selection only on button press (not polling)

**Decision:**
- `getModeForTime()` called **only on button press**, not in main loop

**Reason:**
- RTC optional at boot (can sync later); reduces SD spam if time unavailable
- Saves power by avoiding constant polling
- Mode switch only when user interacts

**Rejected:**
- Continuous time polling in main loop

**Impact:**
- Mode change reflected **after next button press**
- Logs mode switches as system event

---

### 2026-07-03: Power loss resets playback index to 0

**Decision:**
- No persistent state recovery for playback index
- Power loss = restart from track 0 per type

**Reason:**
- EEPROM writes add latency; simplifies state management
- Last click timestamp still logged for recovery context
- Aligns with typical child toy behavior

**Rejected:**
- EEPROM-backed index persistence

**Impact:**
- Playback always resumes from first track after power cycle
- Log shows last activity timestamp for reference

---

### 2026-07-03: Button-to-folder mapping is dynamic and 1:1

**Decision:**
- Available GPIO pins mapped 1:1 to type folders (alphabetically sorted)
- Excess type folders (more folders than pins) are excluded

**Reason:**
- Simplifies button logic; no ambiguity
- Folders discovered dynamically on SD scan
- Alphabetical order predictable for users

**Rejected:**
- Circular cycling through all types
- Multi-button combinations per folder

**Impact:**
- Only 6 button types available (3 direct GPIO + 3 PCF8574)
- Type folders beyond 6 silently excluded from playback

---

### 2026-07-03: Event logging as write queue (not direct SD writes)

**Decision:**
- Events buffered in RAM queue
- Flushed to SD (`/logs/YYYY-MM-DD.json`) when playback idle

**Reason:**
- Avoids log spam during active playback
- Reduces SD wear
- Batches writes for efficiency

**Rejected:**
- Direct SD write per event
- Syslog over network

**Impact:**
- Recent events may not be on SD until playback stops
- Log format: daily JSON rotation (YYYY-MM-DD), reset at midnight
- Serial debug log (optional) for development

---

### 2026-07-05: Re-establish custom I2C pins and add stabilization delays

**Decision:**
- Re-run `extI2C.begin(18, 23)` after RTClib (`rtc.begin()`) and AudioBoardStream (`audioKit->begin()`) initializations.
- Follow every `extI2C.begin()` call with a `delay(50)` to allow the I2C bus lines to stabilize.

**Reason:**
- Third-party libraries call `Wire/Wire1.begin()` internally without arguments during initialization, which resets the custom `TwoWire` instance pins back to microcontroller default hardware pins (away from GPIO 18/23).
- Switching pin configurations via `begin()` causes electrical transitions. Because I2C uses pull-up resistors (open-drain logic), a delay is electrically required to let the SCL and SDA lines pull high and stabilize before executing reads/writes. Without this delay, the ESP32 registers a bus-busy or timeout error (Error 263).

**Rejected:**
- Modifying third-party library source code directly (unmaintainable across updates).
- Omitting the stabilization delay (leads to consistent communication failures on boot).

**Impact:**
- Custom external I2C bus pins are guaranteed to stay routed to GPIO 18/23.
- PCF8574 button expander and DS3231 RTC initialize reliably on boot.
- Developers and LLMs must preserve the `begin()` and `delay(50)` calls after all peripheral initializations.

---

### 2026-07-05: Static OLED Updates and Mystery Quest 24 Constraint

**Decision:**
- Keep OLED track display completely static. Draw track name once at the start of playback, then do not refresh the screen.
- Retain the `u8g2_font_mystery_quest_24_tf` font family for all branding, standby, and track titles.
- Limit track name lengths via `truncateFilename()` to 12 characters to prevent display clipping.

**Reason:**
- **Prevent Audio Buffer Underflow (Stuttering/Lag)**: The OLED is driven on a bit-banged software I2C bus (`SW_I2C`). Sending graphics data over software I2C is CPU-blocking. Frequent/rapid updates (e.g. for dynamic equalizer animations or progress bars) block the audio copy thread, causing underflows in the ES8388 I2S DMA buffers, resulting in audio stuttering and lag.
- **Stylistic Design Requirement**: Mystery Quest 24 font was chosen for branding, and must be preserved instead of utilizing standard sans-serif system fonts.

**Rejected:**
- Real-time progress bars or animated equalizers during active audio playback.
- Pairing Mystery Quest with other font families (e.g. Helvetica/Arial) for body text.

**Impact:**
- Perfect, stutter-free audio playback quality.
- Clean and consistent Mystery Quest aesthetic across all screens.
- Developers and LLMs must not implement periodic screen updates during active playback.
