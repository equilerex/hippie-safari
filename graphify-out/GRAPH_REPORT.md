# Graph Report - hippie safari  (2026-07-11)

## Corpus Check
- 83 files · ~35,770 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 941 nodes · 1253 edges · 120 communities (47 shown, 73 thin omitted)
- Extraction: 92% EXTRACTED · 8% INFERRED · 0% AMBIGUOUS · INFERRED: 94 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `72817abd`
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
- Module 26
- Module 27
- Module 28
- Audio Playback
- Community 30
- Community 31
- 1. External I2C Bus Timeout (Error 263 / ESP_ERR_TIMEOUT)
- Decisions
- PlaybackSessionContext
- Progress
- detectIntent
- ESP32 / PlatformIO Notes
- h
- flushQueue
- graphify.md
- graphify.md
- applypatch-msg
- commit-msg
- husky.sh
- post-applypatch
- post-checkout
- post-commit
- post-merge
- post-rewrite
- pre-applypatch
- pre-auto-gc
- pre-commit
- pre-merge-commit
- pre-push
- pre-rebase
- prepare-commit-msg
- getSystemState
- AudioTools v1.1.3
- Button-to-Type Alphabetical Mapping
- Dual I2C Bus Architecture
- Dynamic Button-to-Folder 1:1 Mapping
- Event Logging as Write Queue
- Mode Selection on Button Press
- Per-Type Variant Index Cycling
- Power Loss Resets Playback Index
- SD Card Content Discovery with Retry
- Three-Level Time Window System
- ConfigLoader
- Content-Driven Architecture
- ContentManager
- Easter Egg Content Support
- ES8388 Audio Codec
- ESP32-A1S Audio Kit v2.2
- Graceful Fallback Pattern
- Audio playback system
- Button input and GPIO management
- Configuration and mode selection system
- Playback state machine and variant rotation
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
- WAV Audio Format Support
- Community 209
- Community 215
- Community 279
- Community 307
- Community 340
- Community 342
- Community 343
- Community 360
- Community 361
- Community 456
- Community 475
- Community 524
- Community 532
- Community 757
- Community 774
- Community 775
- Community 837
- Community 1005

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

## Communities (120 total, 73 thin omitted)

### Community 0 - "Audio Playback"
Cohesion: 0.07
Nodes (26): SemaphoreHandle_t, PCF8574, SystemState, TwoWire, SystemManagerImpl, audioPlayer, audioStoppedMs, buttonMgr (+18 more)

### Community 1 - "Content Discovery"
Cohesion: 0.15
Nodes (21): EasterEggPattern, checkSessionBoundaries, getLastError, getQueueSize, logConfigError, logContentDiscovered, logEasterEggEnded, logEasterEggTriggered (+13 more)

### Community 2 - "Button Input"
Cohesion: 0.05
Nodes (52): ButtonState, ButtonEvent, isPress, pressTimeMs, typeIndex, EasterEggDetector, checkPattern, getLastError (+44 more)

### Community 3 - "System Integration"
Cohesion: 0.10
Nodes (21): RtcManager, getLastError, initialize, isAvailable, now, setTime, RTC_DS3231, time_t (+13 more)

### Community 4 - "Configuration"
Cohesion: 0.09
Nodes (24): DisplayManager, clear, getLastError, initialize, isAvailable, showDebug, showNowPlaying, showStandby (+16 more)

### Community 5 - "System Integration"
Cohesion: 0.08
Nodes (26): AudioStreamState, bytesRead, currentFilePath, isOpen, isPlaying, totalBytes, ContentScan, errorMessage (+18 more)

### Community 6 - "Audio Playback"
Cohesion: 0.13
Nodes (11): string, vector, ModeConfig, modeName, timeWindows, VariantIndex, currentIndex, totalVariants (+3 more)

### Community 8 - "Audio Playback"
Cohesion: 0.06
Nodes (39): TimeWindow, absoluteEndTime, absoluteStartTime, dayMask, endHour, endMin, priority, startHour (+31 more)

### Community 9 - "Audio Playback"
Cohesion: 0.47
Nodes (5): get_cpp_symbols_and_files(), main(), parse_markdown_for_links(), Scans docs/ and root README.md for mentions of C++ symbols and filenames., Scans src/ and include/ to find C++ filenames and declared classes/structs/enums

### Community 10 - "Real-Time Clock"
Cohesion: 0.13
Nodes (14): PlaybackController, enterStandby, exitStandby, getLastError, getVariantIndex, handleButtonEvent, handleEasterEggPattern, initialize (+6 more)

### Community 11 - "Button Input"
Cohesion: 0.08
Nodes (25): RuntimeTuning, allButtonsHeldMs, allButtonsJoinWindowMs, buttonClickMaxHoldMs, buttonDebounceMs, chaosBurstThreshold, chaosBurstWindowMs, eventHistoryWindowMs (+17 more)

### Community 12 - "Audio Playback"
Cohesion: 0.08
Nodes (24): PlaybackLogger, endSession, flushQueue, getLastError, getQueueSize, initialize, logConfigError, logContentDiscovered (+16 more)

### Community 13 - "Content Discovery"
Cohesion: 0.12
Nodes (17): EventType, PlaybackIntent, LogEntry, actualDurationMs, completedFully, context, details, eventType (+9 more)

### Community 15 - "Button Input"
Cohesion: 0.05
Nodes (52): EasterEggState, lastInteractionTime, patternLoopIndex, PlaybackState, currentModeIndex, currentTypeIndex, currentVariantIndex, isPlaying (+44 more)

### Community 16 - "Audio Playback"
Cohesion: 0.21
Nodes (12): TwoWire, scanI2CBus(), checkAndHandleEasterEgg, getContentManager, getLastError, getPlaybackLogger, initialize, initializeSubsystems (+4 more)

### Community 17 - "Button Input"
Cohesion: 0.09
Nodes (38): getEasterEggPatternName(), EasterEggPattern, time_t, parseIso8601Utc(), TypeContent, defaultMode, folderName, modes (+30 more)

### Community 22 - "Audio Playback"
Cohesion: 0.09
Nodes (33): AudioBoardStream, EncodedAudioStream, AudioPlayerImpl, audioKit, copier, currentFile, currentVolume, decoded (+25 more)

### Community 26 - "Module 26"
Cohesion: 0.12
Nodes (16): queue, QueuedEvent, map, SessionState, time_t, PlaybackLoggerImpl, activeSessions, debugLoggingEnabled (+8 more)

### Community 27 - "Module 27"
Cohesion: 0.14
Nodes (13): Amplifier, Architecture, Audio (I2S) - Reserved, Board, Buttons, Codec I2C (Internal) - Reserved, Commands verified, Connected Devices (+5 more)

### Community 28 - "Module 28"
Cohesion: 0.15
Nodes (12): SystemManager, getAudioPlayer, getButtonManager, getConfigLoader, getContentManager, getLastError, getPlaybackController, getPlaybackLogger (+4 more)

### Community 30 - "Community 30"
Cohesion: 0.17
Nodes (11): IAudioPlayer, getLastError, getVolume, initialize, isHealthy, isPlaying, playFile, process (+3 more)

### Community 31 - "Community 31"
Cohesion: 0.17
Nodes (11): ButtonManager, checkEasterEggPattern, getButtonPressed, getLastError, getLastEvent, initialize, isTypeAvailable, lockoutButtonsForEasterEgg (+3 more)

### Community 32 - "1. External I2C Bus Timeout (Error 263 / ESP_ERR_TIMEOUT)"
Cohesion: 0.18
Nodes (10): 1. External I2C Bus Timeout (Error 263 / ESP_ERR_TIMEOUT), 2. Easter Egg Detection Non-functional, Cause, Cause, Fix, Fix, Known Failures, Symptom (+2 more)

### Community 33 - "Decisions"
Cohesion: 0.22
Nodes (8): 2026-07-03: Button-to-folder mapping is dynamic and 1:1, 2026-07-03: Event logging as write queue (not direct SD writes), 2026-07-03: Mode selection only on button press (not polling), 2026-07-03: Power loss resets playback index to 0, 2026-07-03: Separate I2C buses for audio and peripherals, 2026-07-05: Re-establish custom I2C pins and add stabilization delays, 2026-07-05: Static OLED Updates and Mystery Quest 24 Constraint, Decisions

### Community 34 - "PlaybackSessionContext"
Cohesion: 0.29
Nodes (7): PlaybackSessionContext, interruptionDelayMs, modeChanged, pressCount, timeSinceLastMs, variantCycleCount, updateSessionContext

### Community 35 - "Progress"
Cohesion: 0.50
Nodes (3): 2026-07-03, 2026-07-05, Progress

### Community 36 - "detectIntent"
Cohesion: 0.50
Nodes (4): PlaybackIntent, SessionState, detectIntent, endSession

### Community 39 - "flushQueue"
Cohesion: 0.67
Nodes (3): time_t, flushQueue, getCurrentLogPath

### Community 209 - "Community 209"
Cohesion: 0.10
Nodes (44): EasterEggPattern, EasterEggDetectorImpl, addEvent, checkPattern, currentlyPressed, currentPressStartMs, detectAllButtonsHeld, detectAscendingSweep (+36 more)

### Community 215 - "Community 215"
Cohesion: 0.04
Nodes (44): 1. ContentManager, 2. ConfigLoader & RTC Support, 3. PlaybackController, 4. AudioPlayer, 5. ButtonManager, 6. PlaybackLogger, Architecture Overview, Best Practices Checklist (+36 more)

### Community 279 - "Community 279"
Cohesion: 0.09
Nodes (21): Amplifier, Audio (I2S), Board, Bus Architecture, Codec Configuration I²C, Confirmed Hardware Configuration (Verified), Connected devices, Current Project Allocation (+13 more)

### Community 307 - "Community 307"
Cohesion: 0.11
Nodes (17): 1. Functional Requirements, 2. Technical Notes, Audio, Content modes, Content Structure, Design Principles, Engineering Philosophy, General Guidance (+9 more)

### Community 340 - "Community 340"
Cohesion: 0.13
Nodes (14): File Structure, Hippy Safari Architecture Implementation Plan, Self-Review Checklist, Task 10: Documentation — Architecture Overview, Task 1: Content Manager — Discover SD Card Structure, Task 2: Config Loader — Parse Species Config JSON, Task 3: Variant Selector — Pick Variant (Default Fallback), Task 4: Playback State — Track Current Playback (+6 more)

### Community 342 - "Community 342"
Cohesion: 0.14
Nodes (13): Behavior, Constraints, Easter Egg Pattern Detection & Playback, Event Flow, Goal, Implementation Scope, Logging Extensions, Modified Files (+5 more)

### Community 343 - "Community 343"
Cohesion: 0.18
Nodes (10): husky, devDependencies, husky, private, scripts, graph:enrich, graph:link, graph:update (+2 more)

### Community 360 - "Community 360"
Cohesion: 0.15
Nodes (12): Amp Enable, App Baseline, AudioTools board profile, Built-in Buttons, Confirmed Board Details, Critical Audio Config, Environment, ESP32-A1S AudioKit v2.2 — Confirmed Setup (+4 more)

### Community 361 - "Community 361"
Cohesion: 0.15
Nodes (12): 3. Physical / Content Notes, 4. Appendix A — Engineering Notes, Audio, Content, Deployment, Hardware, Hardware, Physical Installation (+4 more)

### Community 456 - "Community 456"
Cohesion: 0.33
Nodes (5): AI Project Context, Memory, Prompts, Tooling, Update rules

### Community 475 - "Community 475"
Cohesion: 0.15
Nodes (18): WebServerManager, getLastError, initialize, update, String, mimeTypeFor(), WebServerManagerImpl, getLastError (+10 more)

### Community 524 - "Community 524"
Cohesion: 0.40
Nodes (4): Building, Hippy Safari, Tuning without rebuilding, What's here

### Community 532 - "Community 532"
Cohesion: 0.50
Nodes (3): AI Project Context, Rules, Structure

## Knowledge Gaps
- **484 isolated node(s):** `husky.sh script`, `fix-headers.sh script`, `initialize`, `playFile`, `stop` (+479 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **73 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `SystemManagerImpl` connect `Audio Playback` to `Button Input`, `System Integration`, `Configuration`, `Audio Playback`, `Audio Playback`, `Real-Time Clock`, `Button Input`, `Audio Playback`, `Button Input`, `Community 209`, `Audio Playback`, `getSystemState`, `Module 26`, `Community 475`, `Module 28`, `Community 30`, `Community 31`?**
  _High betweenness centrality (0.205) - this node is a cross-community bridge._
- **Why does `PlaybackControllerImpl` connect `Button Input` to `Audio Playback`, `Audio Playback`, `Audio Playback`, `Real-Time Clock`, `Audio Playback`?**
  _High betweenness centrality (0.068) - this node is a cross-community bridge._
- **Why does `EasterEggDetectorImpl` connect `Community 209` to `Audio Playback`, `Button Input`, `Audio Playback`?**
  _High betweenness centrality (0.063) - this node is a cross-community bridge._
- **What connects `husky.sh script`, `fix-headers.sh script`, `initialize` to the rest of the system?**
  _510 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Audio Playback` be split into smaller, more focused modules?**
  _Cohesion score 0.07407407407407407 - nodes in this community are weakly interconnected._
- **Should `Button Input` be split into smaller, more focused modules?**
  _Cohesion score 0.05380852550663871 - nodes in this community are weakly interconnected._
- **Should `System Integration` be split into smaller, more focused modules?**
  _Cohesion score 0.09666666666666666 - nodes in this community are weakly interconnected._