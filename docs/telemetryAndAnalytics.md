# Telemetry & Analytics — Concept Notes

Living ideas doc. Not a spec, not an implementation plan. No finalized metric formulas, no firmware schema changes committed here — purely conceptual alignment before deciding what to actually build. Expected to evolve over multiple future passes.

## Hardware/architecture ground truth (as of this pass)

Current:
- 6 total buttons (3 direct GPIO + 3 via PCF8574 I2C expander @0x20), fixed 1:1 alphabetical folder mapping.
- PCF8574 is polled over I2C, not interrupt-per-pin — bounds precision of any "simultaneous press" detection.
- Power loss resets playback index to 0 by design (no EEPROM persistence) — sessions cannot meaningfully continue across a reboot.
- Mode is only re-evaluated on button press, not polled continuously against RTC.
- Logging is a RAM queue flushed only when playback goes idle.
- OLED stays static during playback (bit-banged SW I2C, blocking) — no cheap on-device "live progress" signal exists today; drop-off timing must come from the audio/interrupt layer. (Flagged as possibly a fixable constraint — see `docs/todo.md`.)

Planned:
- ~10 core buttons + 2-3 dedicated single-GPIO easter-egg buttons (directly wired, not pattern-detected).
- Second PCF8574 expander of the same type, extending to 16 available pins total.
- Potentiometer-based volume control.
- Fix: the 5-minute session-silence timeout currently also resets the real easter-egg pattern-detector state — this coupling is a bug (see `docs/todo.md`); the 5min window is meant for analytics session boundaries only.

## Framing corrections (open problems, not resolved facts)

1. **Session ≠ person.** The 5-min-silence session grouping can't distinguish one lingering visitor, a group taking turns, or a stranger silently taking over mid-session.
2. **"Completion" framing is wrong.** Pressing another button is usually reselection, not rejection/abandonment. The current ENGAGED/EXPLORING/REJECTED taxonomy bakes in a value judgment that doesn't hold and needs real expansion.

## Session/identity concepts

- **Occupancy window, not identity**: "session" is a time-boxed occupancy window, nothing more. We cannot accurately distinguish a handoff, a group, or a single lingering person — the 5-min session boundary is the only real, defensible cut we have. No precise handoff/group detection pursued.

## Reframed interaction taxonomy (expanding the "useless 3")

**Flavor only, not scientific.** These archetypes are for fun/storytelling on top of sessions — not a rigorous or accurate classifier. Several share overlapping triggers (e.g. high tempo shows up in both Instrument Player and Codebreaker), so forcing one winning label per session is arbitrary. Use **tags, not buckets**: a session can carry multiple archetype tags at once, or none. This is settled, not open.

Computed off neutral signals (tempo, hold duration, variance, occupancy length) rather than intent labels:

- **The Naturalist** — long uninterrupted single listen, low tempo, settles fast.
- **The Instrument Player** — high tempo, short holds, no long-duration outlier. (Reframe of "Chaos Gremlin" — strips judgment; fast-cycling may be legitimate play, using the box as an instrument.)
- **The Guerilla/Crew** — multi-GPIO concurrent activity or tight-cluster handoff signature; a fun flavor tag only, not a real group/handoff detector (see occupancy-window note above). Caveat: button count is growing (6 today → ~10 core + 2-3 dedicated egg buttons + second expander planned), and PCF8574 is I2C-polled — true simultaneity is bounded by poll cycle time regardless of pin count. Treat "concurrent" as "within one poll window," not literal same-instant.
- **The Sentinel** — long total occupancy, low click density, but not one continuous track (distinct from Naturalist: watches the box, not one story).
- **The Codebreaker** — high press-variance, hold-duration outliers, deliberate-looking rhythm attempts that never trip an egg; most dependent on new raw hold-duration data to distinguish "deliberate" from "random mashing."
- **The Grazer** (new) — moderate tempo, high type/variant diversity, no single long listen, no aggression either. Likely the most common real behavior; currently has no bucket (everyone non-extreme falls into a gap between Naturalist and Instrument Player today).
- **The Loop Rider** (new) — repeatedly returns to the same track/variant within or across sessions; ties to the "second chance rate" metric; distinct from Naturalist because it's about repetition, not one long sit.

## Novel metric ideas

- **Second chance rate**: revisiting previously sampled content.
- **Settle-in latency**: time to first long uninterrupted listen.
- **Egg-adjacent curiosity spikes**: novel button combos tried after an easter egg fires, regardless of success.
- **Discovery-mechanism split**: dedicated single-GPIO egg buttons (planned) are a direct, unambiguous discovery signal (button pressed = found), separate from pattern-mashing-detected eggs which are inherently probabilistic/fuzzy. Worth tracking discovery funnels separately per mechanism type.
- **Organic user-invented rhythm/pattern detection**: crowd inventing patterns that could seed future easter eggs.
- **Week-long novelty/fatigue curve**: day-1 vs day-3+ behavior comparison.
- **Attention drop-off histogram**: 2-second bucket distribution of interrupt time across `PLAYBACK_ENDED` events — reveals editorial dead zones in scripts.
- **Interaction tempo distribution** (renamed from "frustration index" — high tempo isn't necessarily aggression; kept as a neutral distribution, not a verdict).
- **Group Handoff Rate**: proportion of session starts with very short `timeSinceLast` — flavor-level "busy crowd" signal only, not proof of an actual handoff (per occupancy-window note above).

## Special-modes context

- **Mode-behavior correlation**: do archetypes/tempo shift by mode (e.g. more Instrument-Player/Codebreaker behavior at night vs more Naturalist/Grazer in daytime)? Turns mode-usage % into a behavioral lens instead of trivia.
- **Content-mode fit**: cross-reference content-type completion/settle-rate by active mode, to check whether content written for one mode (e.g. "night") actually resonates in its intended time slot vs elsewhere — signal for rebalancing the schedule.

## System integrity concepts (firmware-gated)

- GPIO stress profiling, reboot/heap telemetry.
- **Fallback cascade audit**: dedicated event distinguishing "used fallback variant" from generic error, plus a content-reliability ratio (fallback occurrences vs perfect hits).
- **SD bus saturation report**: time delta between `SD_ERROR` and `SD_RECOVERED`, correlated against interaction-velocity windows to separate hardware SPI contention from unrelated instability.
- **Volume-adjustment telemetry**: confirmed real future feature (potentiometer). Track adjustment events, correlate against time-of-day/mode (e.g. volume spikes during night/party windows).
- **Button hold/release granularity**: raw per-press hold-duration event. This is the highest-leverage new raw data point — most other concepts above (Codebreaker detection, rhythm fingerprinting, GPIO stress) depend on it.

## Filtered out from external ("Gemini") input, with reasons

- Archetype definitions using `REJECTED`/`completions==0` as core signal — rebuilt above off neutral signals instead.
- "Frustration & Aggression Index" framing — renamed to neutral "interaction tempo," judgment dropped.
- Full keymap/enum restatement — not needed; just note new candidate event IDs (volume adjusted, fallback cascade, button released) to merge into the existing keymap later.
- Code snippets / implementation hooks — premature, this doc stays conceptual.

## Open questions

- (none currently — handoff/group precision and archetype tag-vs-bucket questions are settled: not pursued / tags, respectively)

## Non-goals for this doc

No implementation detail, no firmware schema changes, no finalized metric formulas.
