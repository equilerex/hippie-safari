# Hippy Safari

ESP32-A1S audio installation. Press a button, hear a "documentary" narration clip about the animals you're watching. Unattended, weatherproofed, runs on repeat at an event/festival for however long it needs to.

Hidden button-combo easter eggs (holds, sequences, rapid-fire patterns) trigger bonus clips, because a naturalist safari deserves surprises. Mode/time-based content variants (weekday, weekend, festival window) swap tracks without touching firmware — just edit `config.json` files on the SD card.

## What's here

- **Firmware** (`src/`, `include/`): PlatformIO/Arduino C++ for the ESP32-A1S. Buttons → PCF8574 → interrupt-driven event detection → SD-backed audio playback (WAV) → OLED status.
- **Content** (`sd-card-sample/`): example SD card layout — audio files, per-type `config.json` mode configs, and `safari-conf.json` for tuning button/easter-egg timings without a rebuild.
- **Docs** (`docs/`): architecture, hardware notes, implementation history. Start there for the real detail.

## Tuning without rebuilding

Drop a `/safari-conf.json` on the SD root to override button debounce, click/hold thresholds, and every easter-egg pattern's timing window. Missing file or missing keys → compiled-in defaults. See `sd-card-sample/safari-conf.json` for the full key list.

## Building

Standard PlatformIO project. `pio run -t upload -t monitor` to flash + watch serial.
