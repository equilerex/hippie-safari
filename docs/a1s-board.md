# Project Hardware

## Board

- AI-Thinker ESP32-A1S Audio Kit V2.2
- Audio codec: ES8388
- ESP32 module: ESP32-WROVER

---

# Confirmed Hardware Configuration (Verified)

The following has been verified on the actual hardware.

## Audio (I2S)

| Signal | GPIO |
|---------|-----:|
| BCLK | 27 |
| LRCK / WS | 26 |
| DIN | 25 |
| DOUT | 35 |
| MCLK | 0 |

These pins are required for audio playback and should not be repurposed.

---

## Codec Configuration I²C

The audio firmware successfully initializes and plays audio using:

| Signal | GPIO |
|---------|-----:|
| SDA | 33 |
| SCL | 32 |

These pins configure the ES8388 codec.

No I²C devices are visible on this bus using a normal Wire scanner, indicating the audio library likely manages this bus internally.

Do not attach external peripherals here.

---

## External Expansion I²C (Verified)

A dedicated external I²C bus was verified on the expansion header.

| Signal | GPIO |
|---------|-----:|
| SDA | 18 |
| SCL | 23 |

Verified by hardware scan.

### Connected devices

| Address | Device | Notes |
|---------:|--------|------|
| 0x20 | PCF8574 GPIO Expander | Button expansion |
| 0x3C | SSD1306 0.96" OLED | Display module |
| 0x50 | AT24C32 EEPROM | Integrated on DS3231 module |
| 0x68 | DS3231 RTC | Real-time clock |

Example:

```cpp
TwoWire peripherals = TwoWire(1);

peripherals.begin(18, 23);
```

All external peripherals should share this bus.

Recommended usage:

```cpp
display.begin(..., &peripherals);
rtc.begin(&peripherals);
pcf8574.begin(&peripherals);
```

Current peripherals:

```
ESP32 GPIO18 (SDA)
            │
            ├── DS3231 RTC (0x68)
            │      └── AT24C32 EEPROM (0x50)
            │
            ├── SSD1306 OLED (0x3C)
            │
            └── PCF8574 GPIO Expander (0x20)

ESP32 GPIO23 (SCL)
            │
            └── Shared by all devices
```

This bus has been verified to operate correctly alongside audio playback.

---

## OLED Display

### Module

- Controller: **SSD1306**
- Size: **0.96"**
- Interface: **I²C**
- Resolution: **128 × 64**
- I²C Address: **0x3C**

Board markings:

```
OLED IIC
ADD_SELECT
0x78 / 0x7A
```

The PCB uses **8-bit I²C notation**:

| PCB Marking | Actual Address |
|-------------|---------------:|
| 0x78 | 0x3C |
| 0x7A | 0x3D |

This project uses **0x3C**.

---

## SD Card

| Signal | GPIO |
|---------|-----:|
| CS | 13 |
| MISO | 2 |
| MOSI | 15 |
| SCK | 14 |

---

## Amplifier

| Function | GPIO |
|----------|-----:|
| Amplifier Enable / Shutdown | 21 |

```cpp
constexpr bool AMP_ENABLED_STATE = LOW;
```

GPIO21 should not be reused.

---

## Onboard Buttons

| Button | GPIO | Notes |
|--------|-----:|------|
| Key 1 | 36 | Unreliable with Wi-Fi |
| Key 2 | 13 | Shared with SD card |
| Key 3 | 19 | LED conflict |
| Key 4 | 23 | Available if external I²C not used |
| Key 5 | 18 | Available if external I²C not used |
| Key 5 | 18 | Available if external I²C not used |
| Key 6 | 5 | Available |

Current project uses only:

- GPIO5
- GPIO18
- GPIO23

Since GPIO18/23 are now dedicated to the external I²C bus, button expansion is handled using a PCF8574.

---

 
## Expansion Header

### Important

The expansion header exposes **electrical connections to the ESP32 GPIOs**.

An exposed pin is **not necessarily unused**.

Many GPIOs are shared with onboard hardware (audio codec, SD card, amplifier, buttons, etc.). Before assigning a GPIO to a new function, always check whether it is already used elsewhere in this document.

---

### Physical Header

```
3V3
GND

GPIO0
GPIO5
GPIO18
GPIO19
GPIO21
GPIO22
GPIO23

TX0
RX0
RST
```

---

### Current Project Allocation

The project intentionally assigns the exposed GPIOs as follows:

| GPIO | Purpose | Status |
|------|---------|--------|
| GPIO0 | Audio MCLK / Boot strap | Reserved |
| GPIO5 | Onboard debug button | Available |
| GPIO18 | External I²C SDA | Reserved |
| GPIO19 | Spare GPIO | Available |
| GPIO21 | Amplifier enable | Reserved |
| GPIO22 | Spare GPIO | Available |
| GPIO23 | External I²C SCL | Reserved |
| TX0 | Serial | Programming / Debug |
| RX0 | Serial | Programming / Debug |
| 3V3 | Power | External peripherals |
| GND | Ground | External peripherals |
| RST | Reset | Reset input |

---

### External I²C Expansion

GPIO18 and GPIO23 are intentionally dedicated to an external I²C bus.

```
ESP32
GPIO18 ─── SDA
GPIO23 ─── SCL
```

The following peripherals share this bus:

| Address | Device |
|---------:|--------|
| 0x20 | PCF8574 GPIO Expander |
| 0x3C | SSD1306 OLED |
| 0x50 | AT24C32 EEPROM |
| 0x68 | DS3231 RTC |

Multiple I²C devices share the same SDA/SCL wires.

---

### GPIO Expansion (PCF8574)

The PCF8574 provides **8 additional digital GPIO pins** over the I²C bus.

These are **not ESP32 GPIOs**.

```
PCF8574

P0
P1
P2
P3
P4
P5
P6
P7
```

Each pin may be used independently as a digital input or output.

Typical button wiring:

```
P0 ─── Button ─── GND
P1 ─── Button ─── GND
...
P7 ─── Button ─── GND
```

The PCF8574 is the preferred location for all user buttons, allowing the ESP32 GPIOs to remain dedicated to board hardware and communication buses. 
---

# GPIO Usage Summary

| GPIO | Usage | Status |
|------:|-------|--------|
| 0 | MCLK / Boot strap | Reserved |
| 2 | SD MISO | Reserved |
| 5 | Button | Available |
| 13 | SD CS | Reserved |
| 14 | SD CLK | Reserved |
| 15 | SD MOSI | Reserved |
| 18 | External I²C SDA | Reserved |
| 19 | Button / LED | Avoid |
| 21 | Amplifier Enable | Reserved |
| 22 | Currently free | Available |
| 23 | External I²C SCL | Reserved |
| 25 | I²S DIN | Reserved |
| 26 | I²S LRCK | Reserved |
| 27 | I²S BCLK | Reserved |
| 32 | Codec I²C SCL | Internal |
| 33 | Codec I²C SDA | Internal |
| 35 | I²S DOUT | Reserved |
| 36 | Button | Unreliable with Wi-Fi |
| 39 | Headphone Detect | Reserved |

---

## Bus Architecture

The project intentionally uses two independent buses:

| Bus | Purpose |
|-----|---------|
| Internal I²C (GPIO32/33) | ES8388 audio codec configuration |
| External I²C (GPIO18/23) | OLED, RTC, PCF8574 and future peripherals |

Keeping audio and peripherals on separate buses avoids coupling external hardware to the audio subsystem.

---

# Recommended Expansion Strategy

- Use GPIO18/23 exclusively for external I²C.
- Connect all external I²C devices (RTC, OLED, PCF8574, future sensors) to this bus.
- Use the PCF8574 for all additional buttons.
- Leave the internal audio hardware completely untouched.
- consider use of a library fx: TwoWire peripherals = TwoWire(1); peripherals.begin(18, 23);
 
 