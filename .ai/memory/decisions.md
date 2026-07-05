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
