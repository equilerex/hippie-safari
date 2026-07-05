# Known Failures

## None documented yet

Code compiled successfully but not yet tested on hardware (2026-07-03).

### Next verification

Build and test:
```bash
pio run -e esp32-a1s-audiokit
pio run -e esp32-a1s-audiokit -t upload
pio device monitor -e esp32-a1s-audiokit
```

Watch for:
- Audio codec initialization (ES8388 I2C handshake)
- Button debounce on direct GPIO (5) and PCF8574 (0x20)
- SD card access `/audio/types/` scan
- OLED display initialization (if enabled)
- RTC sync (if enabled)
- Event logging to SD `/logs/`
