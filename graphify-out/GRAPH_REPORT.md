# Graph Report - hippie safari  (2026-07-12)

## Corpus Check
- 97 files · ~47,404 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1045 nodes · 1367 edges · 118 communities (56 shown, 62 thin omitted)
- Extraction: 93% EXTRACTED · 7% INFERRED · 0% AMBIGUOUS · INFERRED: 95 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `d260fd2e`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Audio Playback
- Content Discovery
- Button Input
- System Integration
- Configuration
- System Integration
- Audio Playback
- Content Discovery
- Audio Playback
- Audio Playback
- Real-Time Clock
- Button Input
- Audio Playback
- Content Discovery
- Button Input
- Button Input
- Audio Playback
- Button Input
- Audio Playback
- Configuration
- Module 20
- Module 21
- Audio Playback
- System Integration
- Audio Playback
- Event Logging
- Module 28
- Audio Playback
- Community 30
- Community 31
- 1. External I2C Bus Timeout (Error 263 / ESP_ERR_TIMEOUT)
- Progress
- detectIntent
- ESP32 / PlatformIO Notes
- h
- graphify.md
- applypatch-msg
- commit-msg
- husky.sh
- post-applypatch
- post-checkout
- post-rewrite
- pre-applypatch
- pre-commit
- pre-push
- pre-rebase
- prepare-commit-msg
- getSystemState
- debug-bug.md
- esp32-hardware-change.md
- AudioTools v1.1.3
- Dual I2C Bus Architecture
- Event Logging as Write Queue
- Power Loss Resets Playback Index
- SD Card Content Discovery with Retry
- Three-Level Time Window System
- ES8388 Audio Codec
- ESP32-A1S Audio Kit v2.2
- Button input and GPIO management
- Configuration and mode selection system
- Playback state machine and variant rotation
- pre-commit
- pre-commit script
- EventType
- GPIO pin configuration
- Internal I2C (GPIO32/33) for Codec
- Audio Interrupt on Button Press
- Library-First Approach
- PCF8574 GPIO Expander
- Per-Type Variant Index Persistence
- PlaybackController
- PlaybackLogger
- SD Card Content Discovery
- ContentManagerImpl::retrySDMount
- Standby & Recovery State
- Time-Based Mode Selection
- Type-Mode-Variant Hierarchy
- Variant Cycling on Repeat Press
- Progress
- graphify.md
- pre-commit
- sync_agent_docs.py
- Fixed I2C blockage (Error 263)
- Fixed rapid button click loss: interrupt-driven input on GPIO 19 INT pin
- Fully wired and implemented Easter Egg detection and playback
- Audio playback: ES8388 codec via AudioTools StreamCopy
- Button input: 50ms debounce on GPIO polls, content availability filtering
- Config: JSON per mode (time windows: absolute, day-specific, daily recurring)
- Logging: event queue in RAM → SD daily JSON rotation, flush on playback idle
- graphify.md (workflows)
- esp32-hardware-change.md
- implement-feature.md
- review-diff.md
- graphify.config.yaml
- README.md
- p
- e2
- d
- highcharts.js
- ep
- i9
- i
- tu
- .css
- tp
- eH
- iW
- q
- ey
- A
- tx

## God Nodes (most connected - your core abstractions)
1. `SystemManagerImpl` - 55 edges
2. `EasterEggDetectorImpl` - 47 edges
3. `PlaybackLoggerImpl` - 46 edges
4. `ButtonManagerImpl` - 42 edges
5. `PlaybackControllerImpl` - 40 edges
6. `PlaybackLogger` - 30 edges
7. `ContentManagerImpl` - 30 edges
8. `AudioPlayerImpl` - 28 edges
9. `RuntimeTuning` - 25 edges
10. `ContentManager` - 21 edges

## Surprising Connections (you probably didn't know these)
- `logDebugState` --calls--> `getEasterEggPatternName()`  [INFERRED]
  src/EasterEggDetectorImpl.h → include/Config.h
- `handleEasterEggPattern` --calls--> `getEasterEggPatternName()`  [INFERRED]
  src/PlaybackControllerImpl.h → include/Config.h
- `checkAndHandleEasterEgg` --calls--> `getEasterEggPatternName()`  [INFERRED]
  src/SystemManagerImpl.h → include/Config.h
- `AudioPlayerImpl` --inherits--> `IAudioPlayer`  [EXTRACTED]
  src/AudioPlayerImpl.h → include/AudioPlayer.h
- `ButtonManagerImpl` --inherits--> `ButtonManager`  [EXTRACTED]
  src/ButtonManagerImpl.h → include/ButtonManager.h

## Import Cycles
- None detected.

## Communities (118 total, 62 thin omitted)

### Community 0 - "Audio Playback"
Cohesion: 0.14
Nodes (13): Behavior, Constraints, Easter Egg Pattern Detection & Playback, Event Flow, Goal, Implementation Scope, Logging Extensions, Modified Files (+5 more)

### Community 1 - "Content Discovery"
Cohesion: 0.14
Nodes (13): Amplifier, Architecture, Audio (I2S) - Reserved, Board, Buttons, Codec I2C (Internal) - Reserved, Commands verified, Connected Devices (+5 more)

### Community 2 - "Button Input"
Cohesion: 0.18
Nodes (10): 1. External I2C Bus Timeout (Error 263 / ESP_ERR_TIMEOUT), 2. Easter Egg Detection Non-functional, Cause, Cause, Fix, Fix, Known Failures, Symptom (+2 more)

### Community 3 - "System Integration"
Cohesion: 0.04
Nodes (44): 1. ContentManager, 2. ConfigLoader & RTC Support, 3. PlaybackController, 4. AudioPlayer, 5. ButtonManager, 6. PlaybackLogger, Architecture Overview, Best Practices Checklist (+36 more)

### Community 4 - "Configuration"
Cohesion: 0.22
Nodes (8): 2026-07-03: Button-to-folder mapping is dynamic and 1:1, 2026-07-03: Event logging as write queue (not direct SD writes), 2026-07-03: Mode selection only on button press (not polling), 2026-07-03: Power loss resets playback index to 0, 2026-07-03: Separate I2C buses for audio and peripherals, 2026-07-05: Re-establish custom I2C pins and add stabilization delays, 2026-07-05: Static OLED Updates and Mystery Quest 24 Constraint, Decisions

### Community 5 - "System Integration"
Cohesion: 0.40
Nodes (4): Building, Hippy Safari, Tuning without rebuilding, What's here

### Community 6 - "Audio Playback"
Cohesion: 0.10
Nodes (44): EasterEggPattern, EasterEggDetectorImpl, addEvent, checkPattern, currentlyPressed, currentPressStartMs, detectAllButtonsHeld, detectAscendingSweep (+36 more)

### Community 7 - "Content Discovery"
Cohesion: 0.50
Nodes (3): 2026-07-03, 2026-07-05, Progress

### Community 8 - "Audio Playback"
Cohesion: 0.50
Nodes (3): AI Project Context, Rules, Structure

### Community 11 - "Button Input"
Cohesion: 0.09
Nodes (33): AudioBoardStream, EncodedAudioStream, AudioPlayerImpl, audioKit, copier, currentFile, currentVolume, decoded (+25 more)

### Community 12 - "Audio Playback"
Cohesion: 0.09
Nodes (40): getEasterEggPatternName(), EasterEggPattern, ModeConfig, modeName, timeWindows, parseIso8601Utc(), TypeContent, defaultMode (+32 more)

### Community 14 - "Button Input"
Cohesion: 0.14
Nodes (8): string, vector, VariantIndex, currentIndex, totalVariants, typeIndex, PCF8574, map

### Community 15 - "Button Input"
Cohesion: 0.09
Nodes (24): DisplayManager, clear, getLastError, initialize, isAvailable, showDebug, showNowPlaying, showStandby (+16 more)

### Community 19 - "Configuration"
Cohesion: 0.07
Nodes (26): SemaphoreHandle_t, PCF8574, SystemState, TwoWire, SystemManagerImpl, audioPlayer, audioStoppedMs, buttonMgr (+18 more)

### Community 20 - "Module 20"
Cohesion: 0.08
Nodes (25): RuntimeTuning, allButtonsHeldMs, allButtonsJoinWindowMs, buttonClickMaxHoldMs, buttonDebounceMs, chaosBurstThreshold, chaosBurstWindowMs, eventHistoryWindowMs (+17 more)

### Community 21 - "Module 21"
Cohesion: 0.08
Nodes (24): PlaybackLogger, endSession, flushQueue, getLastError, getQueueSize, initialize, logConfigError, logContentDiscovered (+16 more)

### Community 22 - "Audio Playback"
Cohesion: 0.10
Nodes (21): RtcManager, getLastError, initialize, isAvailable, now, setTime, RTC_DS3231, time_t (+13 more)

### Community 24 - "Audio Playback"
Cohesion: 0.15
Nodes (18): WebServerManager, getLastError, initialize, update, String, mimeTypeFor(), WebServerManagerImpl, getLastError (+10 more)

### Community 28 - "Module 28"
Cohesion: 0.09
Nodes (21): Amplifier, Audio (I2S), Board, Bus Architecture, Codec Configuration I²C, Confirmed Hardware Configuration (Verified), Connected devices, Current Project Allocation (+13 more)

### Community 29 - "Audio Playback"
Cohesion: 0.15
Nodes (21): EasterEggPattern, checkSessionBoundaries, getLastError, getQueueSize, logConfigError, logContentDiscovered, logEasterEggEnded, logEasterEggTriggered (+13 more)

### Community 30 - "Community 30"
Cohesion: 0.05
Nodes (52): ButtonState, ButtonEvent, isPress, pressTimeMs, typeIndex, EasterEggDetector, checkPattern, getLastError (+44 more)

### Community 31 - "Community 31"
Cohesion: 0.08
Nodes (26): AudioStreamState, bytesRead, currentFilePath, isOpen, isPlaying, totalBytes, ContentScan, errorMessage (+18 more)

### Community 32 - "1. External I2C Bus Timeout (Error 263 / ESP_ERR_TIMEOUT)"
Cohesion: 0.10
Nodes (19): 10. Multi-Click (Medium), 1. Secret Button (Easy), 2. Ascending Sweep (Easy), 3. SOS Morse (Medium), 4. Hammer Single (Medium), 5. Team Effort (Medium), 6. Long Hold Sustained (Medium), 7. Multi-Hold / Duet (Hard) (+11 more)

### Community 35 - "Progress"
Cohesion: 0.11
Nodes (17): 1. Functional Requirements, 2. Technical Notes, Audio, Content modes, Content Structure, Design Principles, Engineering Philosophy, General Guidance (+9 more)

### Community 37 - "ESP32 / PlatformIO Notes"
Cohesion: 0.12
Nodes (17): EventType, PlaybackIntent, LogEntry, actualDurationMs, completedFully, context, details, eventType (+9 more)

### Community 38 - "h"
Cohesion: 0.12
Nodes (16): queue, QueuedEvent, map, SessionState, time_t, PlaybackLoggerImpl, activeSessions, debugLoggingEnabled (+8 more)

### Community 41 - "graphify.md"
Cohesion: 0.05
Nodes (50): EasterEggState, lastInteractionTime, patternLoopIndex, ConfigLoader, getLastError, isTimeWindowActive, selectModeForTime, PlaybackRequest (+42 more)

### Community 42 - "applypatch-msg"
Cohesion: 0.21
Nodes (12): TwoWire, scanI2CBus(), checkAndHandleEasterEgg, getContentManager, getLastError, getPlaybackLogger, initialize, initializeSubsystems (+4 more)

### Community 43 - "commit-msg"
Cohesion: 0.13
Nodes (14): File Structure, Hippy Safari Architecture Implementation Plan, Self-Review Checklist, Task 10: Documentation — Architecture Overview, Task 1: Content Manager — Discover SD Card Structure, Task 2: Config Loader — Parse Species Config JSON, Task 3: Variant Selector — Pick Variant (Default Fallback), Task 4: Playback State — Track Current Playback (+6 more)

### Community 44 - "husky.sh"
Cohesion: 0.13
Nodes (14): husky, devDependencies, husky, private, scripts, --- agents:sync ---, --- ai:sync ---, --- graph:enrich --- (+6 more)

### Community 45 - "post-applypatch"
Cohesion: 0.13
Nodes (13): ContentManager, discoverContent, discoverEasterEggs, getEasterEggVariantPath, getEasterEggVariants, getLastError, getType, getTypeCount (+5 more)

### Community 46 - "post-checkout"
Cohesion: 0.13
Nodes (14): PlaybackController, enterStandby, exitStandby, getLastError, getVariantIndex, handleButtonEvent, handleEasterEggPattern, initialize (+6 more)

### Community 49 - "post-rewrite"
Cohesion: 0.14
Nodes (13): 🟢 1. Normal Content (`/audio/types/`), 🟡 2. Easter Eggs (`/audio/easter-egg/`), 🔵 3. Testing Playback Modes (Debugging), 📋 Audio Format Specifications (CRITICAL), Creating Easter Eggs, Directory Mapping, File Cycling (Variants), 🎵 Hippy Safari — Audio File Preparation Guide (+5 more)

### Community 50 - "pre-applypatch"
Cohesion: 0.15
Nodes (12): Amp Enable, App Baseline, AudioTools board profile, Built-in Buttons, Confirmed Board Details, Critical Audio Config, Environment, ESP32-A1S AudioKit v2.2 — Confirmed Setup (+4 more)

### Community 52 - "pre-commit"
Cohesion: 0.15
Nodes (12): SystemManager, getAudioPlayer, getButtonManager, getConfigLoader, getContentManager, getLastError, getPlaybackController, getPlaybackLogger (+4 more)

### Community 54 - "pre-push"
Cohesion: 0.09
Nodes (29): time_t, PlaybackState, currentModeIndex, currentTypeIndex, currentVariantIndex, isPlaying, lastInteractionTime, playStartTimeMs (+21 more)

### Community 55 - "pre-rebase"
Cohesion: 0.15
Nodes (12): 3. Physical / Content Notes, 4. Appendix A — Engineering Notes, Audio, Content, Deployment, Hardware, Hardware, Physical Installation (+4 more)

### Community 56 - "prepare-commit-msg"
Cohesion: 0.17
Nodes (11): Filtered out from external ("Gemini") input, with reasons, Framing corrections (open problems, not resolved facts), Hardware/architecture ground truth (as of this pass), Non-goals for this doc, Novel metric ideas, Open questions, Reframed interaction taxonomy (expanding the "useless 3"), Session/identity concepts (+3 more)

### Community 57 - "getSystemState"
Cohesion: 0.17
Nodes (11): IAudioPlayer, getLastError, getVolume, initialize, isHealthy, isPlaying, playFile, process (+3 more)

### Community 58 - "debug-bug.md"
Cohesion: 0.17
Nodes (11): ButtonManager, checkEasterEggPattern, getButtonPressed, getLastError, getLastEvent, initialize, isTypeAvailable, lockoutButtonsForEasterEgg (+3 more)

### Community 59 - "esp32-hardware-change.md"
Cohesion: 0.24
Nodes (11): call_ollama(), extract_classes_from_header(), get_file_hash(), load_env(), main(), Extracts class/struct names and their declarations from a C++ header., Returns the MD5 hash of a file., Loads environment variables from the root .env file. (+3 more)

### Community 62 - "AudioTools v1.1.3"
Cohesion: 0.05
Nodes (33): Agent Instructions Source, AI Project Context, AI Workflow Experiments, Embedded Safety, LLM-Specific Customizations, LLM-Specific Customizations, LLM-Specific Customizations, Memory (+25 more)

### Community 64 - "Dual I2C Bus Architecture"
Cohesion: 0.25
Nodes (7): AI Project Context, AI Workflow Experiments, Embedded Safety, LLM-Specific Customizations, Memory, Tooling, Update rules

### Community 66 - "Event Logging as Write Queue"
Cohesion: 0.36
Nodes (7): find_ollama_log(), main(), monitor_log(), Finds the Ollama server.log file depending on the OS., Tails the Ollama log file and prints parsed generation stats in real-time., Updates the local_llm_savings.json log file and prints a fun savings report., update_savings()

### Community 70 - "SD Card Content Discovery with Retry"
Cohesion: 0.29
Nodes (6): 1. Easter-egg index reset bug, 2. Hardware expansion, 3. Volume control, 4. Blocking logic investigation (OLED vs buttons/audio), 5. General cleanup pass, Todo / Context Tracking

### Community 71 - "Three-Level Time Window System"
Cohesion: 0.29
Nodes (7): PlaybackSessionContext, interruptionDelayMs, modeChanged, pressCount, timeSinceLastMs, variantCycleCount, updateSessionContext

### Community 76 - "ES8388 Audio Codec"
Cohesion: 0.47
Nodes (5): get_cpp_symbols_and_files(), main(), parse_markdown_for_links(), Scans docs/ and root README.md for mentions of C++ symbols and filenames., Scans src/ and include/ to find C++ filenames and declared classes/structs/enums

### Community 80 - "Button input and GPIO management"
Cohesion: 0.50
Nodes (4): PlaybackIntent, SessionState, detectIntent, endSession

### Community 83 - "pre-commit"
Cohesion: 0.67
Nodes (3): time_t, flushQueue, getCurrentLogPath

## Knowledge Gaps
- **561 isolated node(s):** `husky.sh script`, `fix-headers.sh script`, `initialize`, `playFile`, `stop` (+556 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **62 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `SystemManagerImpl` connect `Configuration` to `Audio Playback`, `h`, `graphify.md`, `applypatch-msg`, `Button Input`, `Audio Playback`, `Button Input`, `Button Input`, `post-checkout`, `Audio Playback`, `pre-commit`, `pre-push`, `Audio Playback`, `Audio Playback`, `getSystemState`, `debug-bug.md`, `Community 30`?**
  _High betweenness centrality (0.185) - this node is a cross-community bridge._
- **Why does `PlaybackControllerImpl` connect `graphify.md` to `post-applypatch`, `post-checkout`, `Button Input`, `Configuration`, `Module 21`, `pre-push`?**
  _High betweenness centrality (0.059) - this node is a cross-community bridge._
- **Why does `ButtonManagerImpl` connect `Community 30` to `debug-bug.md`, `Configuration`, `post-applypatch`, `Button Input`?**
  _High betweenness centrality (0.056) - this node is a cross-community bridge._
- **What connects `husky.sh script`, `fix-headers.sh script`, `initialize` to the rest of the system?**
  _584 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Audio Playback` be split into smaller, more focused modules?**
  _Cohesion score 0.14285714285714285 - nodes in this community are weakly interconnected._
- **Should `Content Discovery` be split into smaller, more focused modules?**
  _Cohesion score 0.14285714285714285 - nodes in this community are weakly interconnected._
- **Should `System Integration` be split into smaller, more focused modules?**
  _Cohesion score 0.043478260869565216 - nodes in this community are weakly interconnected._