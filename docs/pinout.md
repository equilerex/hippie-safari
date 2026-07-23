# ESP32-A1S AudioKit — Actual Wired Pin Map

Board: AI-Thinker ESP32-A1S Audio Kit V2.2 (ESP32-WROVER, ES8388 codec)

Source of truth: `include/Config.h` and the manager implementations in `src/` (current firmware), cross-checked 2026-07-22. This supersedes conflicting entries in `docs/a1s-board.md` and `docs/confirmed-setup.md` — see **Corrections** at the bottom.

## ESP32 GPIO Assignments

| GPIO | Function | Bus / Role | Notes |
|-----:|----------|------------|-------|
| 0 | I2S MCLK | Audio codec (I2S) | Also an ESP32 boot-strapping pin |
| 2 | SD MISO | SPI | |
| 5 | OLED SDA | SW_I2C (bit-banged) | `DisplayManagerImpl` — U8G2 software I2C |
| 13 | SD CS | SPI | |
| 14 | SD SCK | SPI | |
| 15 | SD MOSI | SPI | |
| 18 | External I2C SDA | Wire1 (`extI2C`) | RTC (DS3231) + PCF8574 button expander |
| 19 | PCF8574 INT | Digital input, `INPUT_PULLUP`, FALLING-edge interrupt | Not general purpose — wired to the expander's interrupt line |
| 21 | Amplifier enable | Digital output | `HIGH` = amp powered on |
| 22 | OLED SCL | SW_I2C (bit-banged) | `DisplayManagerImpl` |
| 23 | External I2C SCL | Wire1 (`extI2C`) | |
| 25 | I2S DIN | Audio codec | |
| 26 | I2S LRCK / WS | Audio codec | |
| 27 | I2S BCLK | Audio codec | |
| 32 | Internal I2C SCL | Wire0, codec config | Owned by `AudioBoardStream`; no external devices |
| 33 | Internal I2C SDA | Wire0, codec config | Owned by `AudioBoardStream`; no external devices |
| 35 | I2S DOUT | Audio codec | |

## I2C Devices

### External I2C bus — GPIO18 (SDA) / GPIO23 (SCL), `extI2C` / Wire1

| Address | Device | Purpose |
|--------:|--------|---------|
| 0x20 | PCF8574 | Button expander |
| 0x68 | DS3231 | RTC |
| 0x50 | AT24C32 | EEPROM (on the DS3231 module) |

### OLED bus — GPIO5 (SDA) / GPIO22 (SCL), bit-banged SW_I2C

| Address | Device |
|--------:|--------|
| 0x3C | 128×64 OLED (U8G2 `SH1106` driver) |

This is a **separate bus** from the external I2C bus above — it does not share GPIO18/23.

### Internal I2C bus — GPIO32 (SCL) / GPIO33 (SDA)

Managed internally by the audio driver library to configure the ES8388 codec. Not exposed to application code; no external devices attached.

## SPI — SD Card

| Signal | GPIO |
|--------|-----:|
| CS | 13 |
| MISO | 2 |
| MOSI | 15 |
| SCK | 14 |

## PCF8574 Expander Ports (not ESP32 GPIOs — reached over the external I2C bus)

| Port | Function |
|------|----------|
| P0–P6 | Button types (dynamic count, up to 7) |
| P7 | Hidden easter-egg button |

## UART

| Signal | Pin | Purpose |
|--------|-----|---------|
| TX0 / RX0 | default UART0 | Serial programming / debug console (115200 baud) |

---

## History

On 2026-07-22, `docs/a1s-board.md` and `.ai/memory/project-facts.md` were reconciled to match this file — both previously claimed the OLED shared the external I2C bus (GPIO18/23) and listed GPIO5/19/22 as available/spare, which conflicted with the firmware. See the `.ai/memory/decisions.md` entry from that date for details. `PIN_BTN_DEBUG` (GPIO5) in `Config.h` remains dead code — declared but never read anywhere in `src/`.

The OLED driver in code is U8G2's `SH1106` variant; older docs said SSD1306. Address (0x3C) is consistent with either; controller identity wasn't re-verified against the physical part.
