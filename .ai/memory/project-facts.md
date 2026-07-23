# Project Facts

## Stack

- Framework: PlatformIO (ESP-IDF)
- Language: C++
- Package manager: PlatformIO
- Test runner: (Unit tests optional)
- Build tool: `pio run -e esp32-a1s-audiokit`
- Main app entry: `src/main.cpp`

## Architecture

- State: SystemManager (facade) coordinates 7 modules (ContentManager, ConfigLoader, PlaybackController, AudioPlayer, ButtonManager, PlaybackLogger)
- Audio playback: ES8388 codec via AudioTools StreamCopy
- Button input: 50ms debounce on GPIO polls, content availability filtering
- Logging: event queue in RAM → SD daily JSON rotation, flush on playback idle
- Config: JSON per mode (time windows: absolute, day-specific, daily recurring)

## Hardware

### Board
- AI-Thinker ESP32-A1S Audio Kit V2.2
- ESP32-WROVER module
- Codec: ES8388

### Audio (I2S) - Reserved
| Signal | GPIO |
|--------|------|
| BCLK | 27 |
| LRCK/WS | 26 |
| DIN | 25 |
| DOUT | 35 |
| MCLK | 0 |

### Codec I2C (Internal) - Reserved
| Signal | GPIO |
|--------|------|
| SDA | 33 |
| SCL | 32 |

### External I2C (Verified) - Reserved
| Signal | GPIO |
|--------|------|
| SDA | 18 |
| SCL | 23 |

#### Connected Devices
| Address | Device | Purpose |
|---------|--------|---------|
| 0x20 | PCF8574 #1 | Button expansion (chip 0) |
| 0x24 | PCF8574 #2 | Button expansion (chip 1) |
| 0x26 | PCF8574 #3 | Button expansion (chip 2) |
| 0x50 | AT24C32 EEPROM | (on DS3231 module) |
| 0x68 | DS3231 RTC | Real-time clock |

### OLED SW_I2C (Bit-Banged, Separate Bus) - Reserved
| Signal | GPIO |
|--------|------|
| SDA | 5 |
| SCL | 22 |

- Device: U8G2 `SH1106` driver, 128×64, addr 0x3C
- Not on the external I2C bus (GPIO18/23) — its own bit-banged bus

### SD Card (SPI) - Reserved
| Signal | GPIO |
|--------|------|
| CS | 13 |
| MISO | 2 |
| MOSI | 15 |
| SCK | 14 |

### Buttons
- GPIO 5, 22: Reserved (OLED SW_I2C bus, not available as buttons)
- GPIO 18, 23: Reserved (external I2C bus)
- GPIO 19: Reserved (shared PCF8574 interrupt line — all 3 chips' open-drain INT pins are wired together onto this one GPIO; firmware reads all 3 chips on any falling edge, not just one)
- All user buttons: 3x PCF8574 GPIO expanders (0x20 / 0x24 / 0x26), 23 usable ports total (chip 0 gives P0-P6, chips 1-2 give P0-P7 each; chip 0's P7 is reserved for the hidden easter-egg button)
- `NUM_BUTTON_TYPES`/`BUTTON_CHIP_INDEX`/`BUTTON_PORT_INDEX` (Config.h) are set at runtime from SD content discovery, capped at `MAX_BUTTON_TYPES` = 23

### Amplifier
- GPIO 21: Amp enable (HIGH = enabled)

## Commands verified

- Build: `pio run -e esp32-a1s-audiokit`
- Upload: `pio run -e esp32-a1s-audiokit -t upload`
- Monitor: `pio device monitor -e esp32-a1s-audiokit`
- AST Graph Update: `npm run graph:update` (AST-only, run automatically on pre-commit)
- Serena Memory Sync: `npm run serena:sync` (copies .ai facts to Serena and generates C++ class summaries, run automatically on pre-commit)
- Documentation Mapping: `npm run graph:link` (updates C++ symbol-to-documentation mapping)
- Semantic Enrichment (Local LLM): `npm run graph:enrich` (runs deep semantic extraction using qwen2.5-coder:7b via local Ollama on-demand)
- Semantic Enrichment (32B model): `npm run graph:enrich:32b` (runs deep semantic extraction using qwen2.5-coder:32b via local Ollama on-demand)
