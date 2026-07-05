# 🌍 Hippy Safari — Easter Eggs

Hidden moments discovered when visitors interact with the installation in unexpected ways.

## How Easter Eggs Work

The system detects specific button patterns and plays secret narrations, sound effects, or commentary. Some are easy to stumble upon. Some require deliberate discovery. Some might only happen once in a thousand visits — and that's the point.

## The Patterns

### 1. Secret Button (Easy)
**What to do:** Find and press the hidden button.  
**Why:** Curiosity. Exploration. "What's this?"  
**Content:** Mystery reveal. Meta commentary.  
**Folder:** `SECRET_BUTTON/`

### 2. Ascending Sweep (Easy)
**What to do:** Press buttons left-to-right in order, one at a time.  
**Why:** Methodical curiosity. Checking each one.  
**Content:** Ascending notes or structured narration.  
**Folder:** `ASCENDING_SWEEP/`

### 3. SOS Morse (Medium)
**What to do:** Tap pattern: tap-tap-tap (quick), pause, tap-hold-tap-hold-tap-hold, pause, tap-tap-tap.  
**Why:** Distress signal. Communication. "Help!"  
**Content:** Morse code response or animal distress call.  
**Folder:** `SOS_MORSE/`

### 4. Hammer Single (Medium)
**What to do:** Rapidly tap the same button 15+ times in 3 seconds.  
**Why:** Impatience. Obsession. Desperation. "Make it do something!"  
**Content:** Woodpecker on steroids or sarcastic commentary.  
**Folder:** `HAMMER_SINGLE/`

### 5. Team Effort (Medium)
**What to do:** Press all buttons simultaneously.  
**Why:** Group synchronization. Or just sad if you're alone.  
**Content:** "We all showed up!" or bird chorus or awkward silence.  
**Folder:** `TEAM_EFFORT/`

### 6. Long Hold Sustained (Medium)
**What to do:** Press a button and hold for 5+ seconds.  
**Why:** Patience. Meditation. Stubbornness. "I'm not letting go."  
**Content:** Breathing meditation or "And... release."  
**Folder:** `LONG_HOLD_SUSTAINED/`

### 7. Multi-Hold / Duet (Hard)
**What to do:** Press 2+ buttons simultaneously and hold for 3+ seconds.  
**Why:** Coordination. Testing limits. Both hands engaged.  
**Content:** Harmony sound or "Together now..." or duet.  
**Folder:** `MULTI_HOLD/`

### 8. All Buttons Held (Hard)
**What to do:** Press all buttons and hold them all for 5+ seconds.  
**Why:** Ultimate team effort. Full commitment. Desperation.  
**Content:** Epic crescendo or "FULL COMMITMENT!" or cacophony of species.  
**Folder:** `ALL_BUTTONS_HELD/`

### 9. Chaos Burst (Hard)
**What to do:** Tap random buttons as fast as you can for 4 seconds. 12+ presses.  
**Why:** Playful chaos. Testing limits. "What if I just... go nuts?"  
**Content:** Stampede or "Herd behavior detected!" or hysterical nature commentary.  
**Folder:** `CHAOS_BURST/`

### 10. Multi-Click (Medium)
**What to do:** Rapidly tap 2+ different buttons, alternating between them. 4+ presses total in 2 seconds.  
**Why:** Conversation. Back-and-forth. Dialog between participants.  
**Content:** Dueling calls or "They're talking!" or call-and-response narration.  
**Folder:** `MULTI_CLICK/`

## Folder Structure

Each pattern has its own folder named after the pattern:

```
/audio/easter-egg/
  SECRET_BUTTON/
    variant-01.wav
    variant-02.wav
  ASCENDING_SWEEP/
    ascending-notes.wav
  SOS_MORSE/
    sos-response.wav
  HAMMER_SINGLE/
    woodpecker.wav
  TEAM_EFFORT/
    chorus.wav
  LONG_HOLD_SUSTAINED/
    meditation.wav
  MULTI_HOLD/
    duet.wav
  ALL_BUTTONS_HELD/
    crescendo.wav
  CHAOS_BURST/
    stampede.wav
```

## Variant Looping

If an easter egg folder contains multiple `.wav` files:
- First trigger plays `variant-01.wav` (or first file alphabetically)
- Second trigger plays `variant-02.wav` (or next file)
- Loops back to `variant-01.wav` when all are played
- Loop resets after 5 minutes of silence

## Content Notes

- Keep narrations short (5-15 seconds)
- Content should reflect the behavior being observed
- Can be humorous, mysterious, or thematically relevant
- Consider the "nature observation" framing: visitors unknowingly become part of the wildlife being documented

## Example Narrations

**Secret Button:** "Shhh... you found the hidden frequency."

**Ascending Sweep:** "One by one, they make their appearance..."

**SOS Morse:** "The signal has been received."

**Hammer Single:** "Relentless. Determined. Or just impatient?"

**Team Effort:** "And suddenly, everyone arrived at once." *(or if alone)* "...or just one enthusiast, going all-in."

**Long Hold:** "And now, we wait. Patience is a virtue."

**Multi-Hold:** "A moment of synchronization. Rare and beautiful."

**All Buttons Held:** "FULL COMMITMENT."

**Chaos Burst:** "The herd has arrived."

## Technical Notes for Operators

- Easter egg folders are detected at boot if they exist
- New patterns can be added without firmware changes (just create the folder + .wav files)
- The system will automatically cycle through variants on repeated triggers
- If a folder is missing, that pattern is silently ignored (no error)
- Each pattern can have 1-256 variant files

## Discovery Notes

There may be other patterns hidden in the system. Experiment. Listen. Observe. The installation is watching you watching the animals.

---

**Installation Note:** This is an easter egg system. These content additions are optional and purely for surprise and delight. The core content (species, modes, narration) remains the primary focus.
