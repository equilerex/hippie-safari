I 
## 3. Physical / Content Notes

### Physical Installation

* Observation station integrated into a coffee shop or similar structure.
* Designed for unattended operation throughout the event.
* Electronics protected from weather and visitor access.
* Maintenance access limited to operators.

### Hardware

* ESP32-A1S Audio Kit is the planned controller, subject to prototype verification.
* Button LEDs are hardware-driven and are not controlled by firmware.
* Final button count depends on the finished installation.
* Physical button layout is a construction decision.
* Speaker placement and quantity should be determined through real-world testing.

### Content

* Documentary / naturalist narration throughout.
* Every recording should stand on its own.
* Visitors should be able to start listening at any point.
* Species, easter eggs, names, button labels and narration remain content decisions.
* New content should be organised so it can be added without firmware changes.

---

## 4. Appendix A — Engineering Notes

Reference only.

The following are engineering observations, prototype findings and implementation notes. They are not project requirements and should not be treated as fixed architecture. They exist to save future investigation time and should be updated only from real testing.

### Hardware

* Verify the exact ESP32-A1S board revision before development.
* Verify which audio codec, such as AC101 / ES8388 or equivalent, the board contains before selecting software support.
* Start firmware development from a known working manufacturer/demo audio example rather than from scratch.
* Maintain a manually verified pin map. Do not rely on AI-generated GPIO availability tables.

### Audio

* Validate the complete audio path before implementing project logic.
* Preferred output path is 3.5 mm audio into a Bluetooth transmitter feeding one or more Bluetooth speakers.
* Test real-world volume before adding amplifiers or changing hardware.
* Record any confirmed codec quirks or library limitations discovered during testing.

### Storage

* Prototype SD-card content loading before building higher-level logic.
* Runtime state may be cached or persisted if beneficial, but the installation should not become dependent on fragile storage behaviour.
* Keep content structure flexible so new species and variants can be added without firmware changes.

### Time

* RTC is the preferred source of persistent time.
* Network time is optional and should improve accuracy rather than enable core functionality.
* Store timestamps using standard UTC formats.

### Reliability

* Prototype first, optimise later.
* Perform extended real-hardware soak testing before deployment.
* Test repeated power interruptions and recovery behaviour.
* Record any confirmed failure modes together with their verified fixes.

### Software

* Prefer existing, well-tested libraries for common functionality.
* Keep implementation simple until profiling demonstrates a need for optimisation.
* Treat AI-generated implementation suggestions as hypotheses to verify, not as design decisions.

### Deployment

* Maintain a deployment checklist separate from the requirements.
* Keep backup copies of firmware, configuration and audio content.
* Record any operational lessons learned after the event for future iterations.

I would keep this as a single “project notes” doc. If you want it cleaner for implementation handoff, the next useful cut is splitting Section 1 into `functional-requirements.md`, Section 2 into `technical-notes.md`, Section 3 into `physical-content-notes.md`, and Section 4 into `engineering-appendix.md`.
