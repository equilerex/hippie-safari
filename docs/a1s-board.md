# Project hardware:


The AI-Thinker ESP32-A1S V2.2 Audio Kit utilizes an onboard ESP32-WROVER module paired with an ES8388 (or legacy AC101) audio codec. Many GPIOs are reserved for audio. [1, 2, 3]
Here is the specific mapping and usage constraints:
## Audio Codec (ES8388)

* I2S SCK / BCLK: GPIO 27
* I2S WS (LRCK): GPIO 26
* I2S SDIN: GPIO 25
* I2S SDOUT: GPIO 35
* I2S MCLK: GPIO 0
* I2C SDA: GPIO 33
* I2C SCL: GPIO 32 [3, 4]

## Onboard Hardware Connections

* Headphone Jack Detection: GPIO 39
* Amp Shutdown: GPIO 21
* Key 1: GPIO 36
* Key 2 / Vol Up: GPIO 13
* Key 3 / Vol Down: GPIO 19
* Key 4: GPIO 23
* Key 5: GPIO 18
* Key 6: GPIO 5 [1, 5, 6, 7, 8]

## Exposed Header Pins (Top of Board)
The main expansion headers provide:

* Power: 3.3V and GND
* UART: GPIO 1 (TX0) and GPIO 3 (RX0)
* General I/O: GPIO 0, 5, 18, 19, 21, 22, 23
* Strapping Pins: GPIO 0 is tied to the BOOT button. Avoid pulling it LOW at startup to prevent bootlooping. [1, 9, 10]

For an in-depth pin breakdown and layout visual, consult the [TinyTronics ESP32-A1S Specification PDF](https://www.tinytronics.nl/product_files/003625_ESP32-A1S-datasheet.pdf). [11]
 

## Possibly usable pins
----

On the ESP32-A1S V2.2 Audio Kit, almost every single GPIO is already routed to internal hardware (Audio Codec, SD Card reader, onboard buttons, amplifier, and status LEDs).
Because of this heavy hardware integration, there are no truly free, unassigned GPIO pins available on the board. [1]
To connect extra hardware like displays, sensors, or rotary encoders, you have to "steal" pins from the onboard components depending on what features you are willing to disable: [2]
## Option 1: The Best Candidate Pins (Steal from the Onboard Buttons)
The absolute easiest way to free up GPIOs is to avoid using the physical tactile buttons (Key 1 through Key 6) on the board. If you do not press these buttons in your code, their respective pins become available on the expansion header: [2, 3]

* GPIO 22: Originally paired with Key 6 (or standard I2C SCL depending on the exact board revision trace). This is highly recommended for adding external components.
* GPIO 23: Tied to Key 4. Free to use if you don't use that button.
* GPIO 18: Tied to Key 5. Free to use if you don't use that button.
* GPIO 19: Tied to Key 3 / Vol Down. Free to use if you don't use that button.
* GPIO 5: Tied to Key 6 on alternative firmware assignments. Free to use if the button is ignored. [2, 4, 5]

## Option 2: The Digital Communication Pins (I2C)

* GPIO 32 (SCL) & GPIO 33 (SDA): These pins drive the I2C communication to configure the ES8388 audio codec chip. Because I2C is a shared bus, you can safely connect other I2C devices (like an OLED display or an environment sensor) to these same pins without breaking the audio functionality. Just ensure the new devices have a different I2C address than the codec. [6, 7, 8]

## Option 3: Steal from the SD Card Reader
If your project streams audio over Wi-Fi/Bluetooth and does not require an SD card, you can reclaim the entire SPI bus routed to the MicroSD slot: [3, 9]

* GPIO 14 (CLK)
* GPIO 15 (CMD)
* GPIO 2 (D0)
* GPIO 4 (D1)
* GPIO 12 (D2)
* GPIO 13 (D3) [10, 11]

## Pins to Strictly Avoid

* GPIO 25, 26, 27, 35: Do not use these under any circumstance if you want audio to work; they are hardwired to the essential I2S audio stream.
* GPIO 21: Hardwired to the onboard speaker amplifier shutdown circuit. Messing with this will turn your speakers off.
* GPIO 0: A strapping pin tied to the boot button. Pulling this low during a restart will throw the ESP32 into a permanent bootloop. [2, 12, 13]

Which external peripheral are you looking to add? If you tell me what device you want to connect, I can tell you exactly which "stolen" pins will work best for it.
 