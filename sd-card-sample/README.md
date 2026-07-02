# SD Card Sample Structure

This folder shows the correct SD card layout for Hippy Safari.

## Layout

```
audio/
  types/
    01_intro/           # Type 0: Button 1 (GPIO 36)
      config.json       # Mode configuration
      intro_1.wav       # Variant 1
      intro_2.wav       # Variant 2
      ...
    02_plain-clothes/   # Type 1: Button 2 (GPIO 13)
      config.json
      clothes_1.wav
      clothes_2.wav
      ...
    03_groups/          # Type 2: Button 3 (GPIO 19)
      config.json
      group_1.wav
      group_2.wav
      ...
  easter-egg/          # Generic easter egg folder (optional)
    secret.wav
    hidden.wav
logs/
  2026-07-03.json      # Daily log rotation (created by system)
  2026-07-04.json
  ...
```

## How to Use

1. Copy this `sd-card-sample/` folder structure to your SD card
2. Add .wav audio files to each `types/XX_*/` folder
3. Modify `config.json` files to set your time windows and modes
4. Insert SD card into ESP32 and power on

## Button Mapping

Buttons map to type folders in alphabetical order:
- **Button 1 (GPIO 36)** → `01_intro/`
- **Button 2 (GPIO 13)** → `02_plain-clothes/`
- **Button 3 (GPIO 19)** → `03_groups/`
- **Button 4 (GPIO 23)** → (add `04_*` folder)
- **Button 5 (GPIO 18)** → (add `05_*` folder)
- **Button 6 (GPIO 5)** → (add `06_*` folder)

## Config.json Format

Each type can have multiple modes with different time windows:

```json
{
  "modes": [
    {
      "name": "mode_name",
      "timeWindows": [
        {
          "startTime": "HH:MM",
          "endTime": "HH:MM",
          "priority": 0
        }
      ]
    }
  ]
}
```

### Time Window Types

**Daily Recurring** (every day):
```json
{"startTime": "09:00", "endTime": "17:00", "priority": 0}
```

**Day-Specific** (specific days of week):
```json
{"days": ["friday", "saturday"], "startTime": "20:00", "endTime": "06:00", "priority": 5}
```

**Absolute Event** (specific date range):
```json
{"startDateTime": "2026-08-15T14:00:00Z", "endDateTime": "2026-08-17T22:00:00Z", "priority": 100}
```

Higher priority wins if multiple modes match the current time.

## Audio Files

- Format: **WAV only** (no MP3)
- Naming: Alphabetical order determines playback order (01_, 02_, etc.)
- Multiple presses on same button cycle through variants
- Different button resets cycle to first variant

## Logging

- System creates `/logs/YYYY-MM-DD.json` automatically
- One file per day, rotates at midnight
- Log events: button presses, playback start/end, mode changes, errors
