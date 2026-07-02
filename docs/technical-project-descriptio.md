# Hippy Safari — Project Notes

## 1. Functional Requirements

### Project Overview

Hippy Safari is an interactive festival installation where visitors observe festival-goers through a safari viewing station while triggering humorous naturalist-style narration using physical buttons.

The system is entirely content-driven. Adding, removing or changing content should require as little firmware work as possible.

### Visitor Interaction

The installation provides three categories of content:

* **types** – the primary narration buttons.
* **Intro** – presented to visitors as a dedicated start button, but internally treated as a normal types to avoid unnecessary special-case firmware.
* **Easter Eggs** – hidden or alternative interactions outside the normal types selection.

Each button press immediately starts playback, interrupting any current narration.

types and easter eggs may contain multiple recordings. Repeated activations cycle through the available recordings rather than always replaying the same one.

Optional hidden interactions, such as simultaneous button presses, may trigger additional easter egg content.

### Content modes

Each content item may provide multiple variants and modes.

Examples include:

* `default` — always available fallback
* `friday` — 07:00 Friday → 06:00 Saturday
* `night` — 22:00 → 06:00 (time-range based)

Mode selection is time-range based, defined in mode-level `config.json`:

```json
{
  "startTime": "07:00",
  "endTime": "06:00",
  "priority": 1
}
```

Multiple modes may be active simultaneously; highest-priority matching mode is selected. Variant selection is configuration-driven rather than hardcoded to specific names.

### Graceful Behaviour

The installation should continue operating whenever possible.

Examples:

* Missing optional variant → use the default variant.
* Missing optional mode → continue normal playback.
* Unavailable network time → continue using RTC or default behaviour.
* Playback index outside available recordings → restart from the first recording.
* Unavailable optional services → continue core functionality.

Visitors should always experience a working installation even when optional features are unavailable.

### Design Principles

* Minimise special-case firmware.
* Keep firmware independent of individual types and modes.
* New content should be addable without firmware changes.
* Prefer graceful fallbacks over visible failures.
* Prioritise installation stability over additional features.

---

## 2. Technical Notes

### Engineering Philosophy

This is a one-off art installation.

Prefer simple, robust solutions over reusable architectures or premature optimisation.

Prototype first. Optimise only when testing demonstrates a genuine need.

Use established libraries wherever practical instead of reinventing common functionality.

### Content Structure

Recommended layout:

```text
/audio/
    types/

        <type>_intro/ 
            <mode>default/
            ...

        <type>plain-clothes/
            <mode>default/
                <variant>hippie.wav
                <variant>raver.wav
            <mode>friday/
                config.json
                <variant>hippie.wav
                <variant>raver.wav
            ...

        groups/
            default/
                hedonists.wav
                real-working-heroes.wav

    easter-egg/ 
        curious-cat/ 
            default/
                what-ya-doing.wav
            friday/
                what-ya-doing.wav 
```

Every folder inside `types/` represents one normal selectable button.

`_intro` exists only to sort first alphabetically and behaves exactly like every other types.

Every content folder may contain any number of optional mode folders.

The firmware discovers folders automatically rather than containing a hardcoded list of types or modes.

###  Mode-Level Configuration

```text
types/
    plain-clothes/
        default/
           // no config.json (always active)
        friday/
            config.json
               {
                 "startTime": "07:00",
                 "endTime": "06:00",
                 "priority": 1
               }
            hippie.wav
            raver.wav
        night/
            config.json
               {
                 "startTime": "22:00",
                 "endTime": "06:00",
                 "priority": 2
               }
            hippie.wav
            raver.wav
```

**Mode Config Fields:**
- `startTime`, `endTime` — HH:MM format (24-hour); ranges wrap midnight
- `priority` — integer; higher wins when multiple modes overlap
- Other fields — generic key-value for effects, volume, or effects settings

**Pros**

* Fully self-contained time ranges.
* Easy to copy, remove or distribute individual modes.
* Greater flexibility for time-based variant selection.

**Cons**

* More duplicated configuration.
* Requires precedence rules (priority field) when ranges overlap.

### Playback Behavior

Button presses cycle through variants within the current mode:

* **First press on type:** Start playback at variant index 0 of type's active mode
* **Repeat press on same type:** Cycle to next variant (index increments, wraps at end)
* **Press different type:** Reset to index 0, switch to new type
* **Track end:** Reset index to 0, stop playback
* **Index out of bounds (graceful fallback):** Reset to 0, play variant 0
* **Missing variant file:** Try next variant; if all missing, silent fallback to standby

Index is retained per type (not per mode). Switching modes within same type resets index to 0.

### Runtime Behaviour

Implementation details are intentionally left open.

The firmware may:

* Discover folders during boot.
* Cache discovered information.
* Refresh content when appropriate.
* Persist runtime state if useful.
* Log usage statistics (tracks played fully, type switches, recovery events, errors)

The implementation should favour simplicity and reliability over cleverness.

### Logging & Usage Statistics

The system logs usage data for analysis:

* **Track completion:** When variant playback finishes completely
* **Type switches:** Count of button presses switching between types
* **Variant cycles:** Number of times same type was pressed again (cycling)
* **Recovery events:** Graceful fallbacks (missing files, index resets, mode mismatches)
* **System events:** Boot, shutdown, configuration changes
* **Error events:** Failed loads, corrupted audio, I2S errors (recovered)

Logs stored in SPIFFS with timestamps. Can be retrieved for operational analysis post-event.

### Time

Prefer an RTC for persistent local time.

Network time is optional and should improve accuracy rather than enable core functionality.

Store timestamps using standard UTC formats.

### Audio

Preferred signal path:

```text
ESP32-A1S → 3.5 mm audio output → Bluetooth transmitter → Bluetooth speaker(s)
```

A local wired speaker may be retained as backup.

Additional amplification should only be introduced if testing demonstrates a genuine need.

### General Guidance

* Avoid unnecessary special cases.
* Avoid hardcoded content assumptions.
* Separate static firmware configuration from editable runtime/content configuration.
* Graceful recovery is preferable to strict failure.
* Stable behaviour is more important than feature count.
 