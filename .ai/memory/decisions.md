# Decisions

### 2026-07-23: Multi-chip PCF8574 button expansion (3x chips, shared INT line)

**Decision:**
- Added two more PCF8574 GPIO expanders (addr 0x24, 0x26) alongside the original (0x20) on the external I2C bus, to support more than 7 button-mapped content folders (user now has 10).
- All 3 chips' open-drain INT pins are wired together onto the existing GPIO19 line rather than giving each chip its own interrupt GPIO. `ButtonManagerImpl::onPCF8574Interrupt()` now reads all 3 chips (`readAllChips()`) on every falling edge and figures out which bit(s) actually changed, since any chip could have pulled the shared line low.
- `BUTTON_CHIP_INDEX[]` / `BUTTON_PORT_INDEX[]` (Config.h, built once in `SystemManagerImpl.cpp` via `buildButtonPinMap()`) replace the old single `BUTTON_PINS[]` port-only array, mapping each logical button type to a (chip, port) pair. `MAX_BUTTON_TYPES` is now `(NUM_PCF8574_CHIPS * PCF8574_PORTS) - 1` = 23 (one port, chip 0/P7, stays reserved for the secret button).
- `PIN_EASTER_EGG` (the logical index fed to `EasterEggDetectorImpl`) changed from `7` to `0xFF`. With buttons now able to occupy index 7 for real (chip 1, port 0), reusing `7` as the secret-button sentinel would have collided with an actual content button.

**Reason:**
- User physically added the two extra expanders and jumpered A0-A2 to get unique addresses (0x24, 0x26 — confirmed via live `[I2C SCAN]` boot output, no collision with 0x20/0x50/0x68) and tied all 3 INT pins together, matching how PCF8574 INT is meant to be wire-ORed across multiple chips on one bus.

**Rejected:**
- Giving each new chip its own ESP32 GPIO for INT — no spare GPIOs documented as free (see `docs/pinout.md`), and the shared open-drain line already works correctly once firmware reads every chip on each trigger.

**Impact:**
- `ButtonManagerImpl` constructor now takes `PCF8574* chips[NUM_PCF8574_CHIPS]` instead of a single pointer; `SystemManagerImpl` holds `pcf8574[NUM_PCF8574_CHIPS]`.
- Any future PCF8574 must be added to `PCF8574_ADDRESSES[]` and `NUM_PCF8574_CHIPS` (Config.h) — the port/chip mapping and `MAX_BUTTON_TYPES` follow automatically.
- See `known-failures.md` #0 for the two bugs this work surfaced and fixed (hardcoded `NUM_BUTTON_TYPES`, out-of-bounds read in `hasInterferingEvent`).

---

### 2026-07-22: Corrected stale OLED bus / GPIO docs to match firmware

**Decision:**
- Docs (`docs/a1s-board.md`, `.ai/memory/project-facts.md`) previously claimed the OLED shares the external I²C bus (GPIO18/23) alongside the RTC and PCF8574. The firmware (`DisplayManagerImpl`) actually drives the OLED on its own bit-banged SW_I²C bus, GPIO5 (SDA) / GPIO22 (SCL) — confirmed by reading the current source, not just docs.
- Docs also listed GPIO19 as available/"LED conflict" and GPIO5/22 as spare button GPIOs. In the current firmware, GPIO19 is wired to the PCF8574 interrupt line (`INPUT_PULLUP` + `attachInterrupt`), and GPIO5/22 are consumed by the OLED bus. None of these are free.
- `PIN_BTN_DEBUG` (GPIO5) in `Config.h` is dead code — declared but never read anywhere in `src/`.
- Docs and memory updated to reflect three independent buses: internal I2C (audio codec, GPIO32/33), external I2C (RTC + PCF8574, GPIO18/23), and OLED SW_I2C (GPIO5/22).

**Reason:**
- The docs had drifted from the firmware, likely left over from when the OLED was originally planned/prototyped on the shared external I2C bus before being moved to its own bit-banged bus. Found while producing `docs/pinout.md`, a from-source pinout reference.

**Impact:**
- `docs/a1s-board.md`, `.ai/memory/project-facts.md`, and `docs/pinout.md` now agree with the firmware.
- GPIO5, GPIO19, and GPIO22 must not be reused for new buttons/peripherals — see `docs/pinout.md` for the authoritative current map.

---

### 2026-07-22: Amplifier enable polarity corrected to active-HIGH

**Decision:**
- `PIN_AMP_ENABLE` (GPIO21) must be driven HIGH to power the speaker amplifier.
- Previous `AMP_ENABLED_STATE = LOW` was incorrect and has been corrected in `include/Config.h`, `docs/a1s-board.md`, and `project-facts.md`.

**Reason:**
- Verified against actual hardware behavior: HIGH is required to give power to the speaker outputs (amp stays off/muted at LOW, contrary to the earlier documented assumption).

**Rejected:**
- Keeping `LOW` as the enabled state (previously documented, now confirmed wrong).

**Impact:**
- No code path changes other than the constant; `AudioPlayerImpl::initialize()` already calls `digitalWrite(PIN_AMP_ENABLE, AMP_ENABLED_STATE)` and now correctly enables the amp.
- Any other reference to "amp enable = LOW" in docs/memory is stale and should be treated as superseded by this entry.

---

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
