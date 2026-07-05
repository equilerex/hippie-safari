# Graph Report - .  (2026-07-05)

## Corpus Check
- Corpus is ~33,872 words - fits in a single context window. You may not need a graph.

## Summary
- 487 nodes · 733 edges · 30 communities (20 shown, 10 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 47 edges (avg confidence: 0.81)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Content Discovery|Content Discovery]]
- [[_COMMUNITY_Button Input|Button Input]]
- [[_COMMUNITY_System Integration|System Integration]]
- [[_COMMUNITY_Configuration|Configuration]]
- [[_COMMUNITY_System Integration|System Integration]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Content Discovery|Content Discovery]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Real-Time Clock|Real-Time Clock]]
- [[_COMMUNITY_Button Input|Button Input]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Content Discovery|Content Discovery]]
- [[_COMMUNITY_Button Input|Button Input]]
- [[_COMMUNITY_Button Input|Button Input]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Button Input|Button Input]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Configuration|Configuration]]
- [[_COMMUNITY_Module 20|Module 20]]
- [[_COMMUNITY_Module 21|Module 21]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_System Integration|System Integration]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Event Logging|Event Logging]]
- [[_COMMUNITY_Module 26|Module 26]]
- [[_COMMUNITY_Module 27|Module 27]]
- [[_COMMUNITY_Module 28|Module 28]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]

## God Nodes (most connected - your core abstractions)
1. `SystemManagerImpl` - 45 edges
2. `PlaybackLoggerImpl` - 33 edges
3. `PlaybackControllerImpl` - 29 edges
4. `AudioPlayerImpl` - 28 edges
5. `PlaybackLogger` - 27 edges
6. `ContentManagerImpl` - 25 edges
7. `ButtonManagerImpl` - 23 edges
8. `ContentManager` - 22 edges
9. `DisplayManagerImpl` - 21 edges
10. `SystemManager` - 20 edges

## Surprising Connections (you probably didn't know these)
- `SystemManagerImpl::initialize` --semantically_similar_to--> `Dual I2C Bus Architecture`  [INFERRED] [semantically similar]
  D:/ESP32 projects/hippie safari/src/SystemManagerImpl.cpp → D:/ESP32 projects/hippie safari/.ai/memory/decisions.md
- `ContentManagerImpl::discoverContent` --semantically_similar_to--> `Dynamic Button-to-Folder 1:1 Mapping`  [INFERRED] [semantically similar]
  D:/ESP32 projects/hippie safari/src/ContentManagerImpl.cpp → D:/ESP32 projects/hippie safari/.ai/memory/decisions.md
- `PlaybackControllerImpl::notifyClipFinished` --semantically_similar_to--> `Power Loss Resets Playback Index`  [INFERRED] [semantically similar]
  D:/ESP32 projects/hippie safari/src/PlaybackControllerImpl.cpp → D:/ESP32 projects/hippie safari/.ai/memory/decisions.md
- `Audio playback system` --conceptually_related_to--> `IAudioPlayer`  [INFERRED]
  src/AudioPlayerImpl.h → include/AudioPlayer.h
- `Button input and GPIO management` --conceptually_related_to--> `ButtonManager`  [INFERRED]
  src/ButtonManagerImpl.h → include/ButtonManager.h

## Import Cycles
- None detected.

## Communities (30 total, 10 thin omitted)

### Community 0 - "Audio Playback"
Cohesion: 0.05
Nodes (37): EventType, PlaybackLogger, flushQueue, getLastError, getQueueSize, initialize, logConfigError, logEvent (+29 more)

### Community 1 - "Content Discovery"
Cohesion: 0.09
Nodes (38): EventType, LogEntry, details, eventType, modeIndex, timestamp, typeIndex, variantIndex (+30 more)

### Community 2 - "Button Input"
Cohesion: 0.07
Nodes (34): ButtonState, SD card content discovery and caching, ContentManager, discoverContent, getLastError, getType, getTypeCount, getVariantCount (+26 more)

### Community 3 - "System Integration"
Cohesion: 0.08
Nodes (32): SystemState, PCF8574, SemaphoreHandle_t, SystemState, TwoWire, SystemManagerImpl, audioPlayer, buttonMgr (+24 more)

### Community 4 - "Configuration"
Cohesion: 0.09
Nodes (29): Configuration and mode selection system, ConfigLoader, getLastError, isTimeWindowActive, loadTypeConfig, selectModeForTime, time_t, vector (+21 more)

### Community 5 - "System Integration"
Cohesion: 0.08
Nodes (26): DisplayManager, clear, getLastError, initialize, isAvailable, showDebug, showNowPlaying, showStandby (+18 more)

### Community 6 - "Audio Playback"
Cohesion: 0.15
Nodes (12): string, vector, ModeConfig, modeName, timeWindows, TimeWindowType, VariantIndex, currentIndex (+4 more)

### Community 7 - "Content Discovery"
Cohesion: 0.13
Nodes (27): TypeContent, defaultMode, folderName, modes, typeIndex, variants, ContentManagerImpl, cachedTypes (+19 more)

### Community 8 - "Audio Playback"
Cohesion: 0.11
Nodes (24): AudioBoardStream, EncodedAudioStream, AudioPlayerImpl, audioKit, copier, currentFile, currentVolume, decoded (+16 more)

### Community 9 - "Audio Playback"
Cohesion: 0.07
Nodes (26): Playback state machine and variant rotation, time_t, PlaybackState, currentModeIndex, currentTypeIndex, currentVariantIndex, isPlaying, lastInteractionTime (+18 more)

### Community 10 - "Real-Time Clock"
Cohesion: 0.11
Nodes (23): TimeWindow, absoluteEndTime, absoluteStartTime, dayMask, endHour, endMin, priority, startHour (+15 more)

### Community 11 - "Button Input"
Cohesion: 0.09
Nodes (21): Button input and GPIO management, ButtonManager, getButtonPressed, getLastError, getLastEvent, initialize, isTypeAvailable, poll (+13 more)

### Community 12 - "Audio Playback"
Cohesion: 0.10
Nodes (19): Audio playback system, IAudioPlayer, getLastError, getVolume, initialize, isHealthy, isPlaying, playFile (+11 more)

### Community 13 - "Content Discovery"
Cohesion: 0.10
Nodes (20): ContentScan, errorMessage, folderName, success, typeIndex, variantPaths, ContentState, contentDiscovered (+12 more)

### Community 14 - "Button Input"
Cohesion: 0.11
Nodes (19): ButtonManager, Button-to-Type Alphabetical Mapping, Content-Driven Architecture, ContentManager, Easter Egg Content Support, External I2C (GPIO18/23) for Peripherals, Graceful Fallback Pattern, Audio Interrupt on Button Press (+11 more)

### Community 15 - "Button Input"
Cohesion: 0.18
Nodes (11): Event Logging as Write Queue, Mode Selection on Button Press, Per-Type Variant Index Cycling, Three-Level Time Window System, ConfigLoaderImpl::isTimeWindowActive, ConfigLoaderImpl::selectModeForTime, ContentManagerImpl::retrySDMount, PlaybackControllerImpl::getNextVariantIndex (+3 more)

### Community 16 - "Audio Playback"
Cohesion: 0.22
Nodes (8): Dual I2C Bus Architecture, Power Loss Resets Playback Index, loop(), setup(), PlaybackControllerImpl::notifyClipFinished, PlaybackLoggerImpl::flushQueue, SystemManagerImpl::initialize, SystemManagerImpl::update

### Community 17 - "Button Input"
Cohesion: 0.25
Nodes (8): Dynamic Button-to-Folder 1:1 Mapping, SD Card Content Discovery with Retry, ContentManagerImpl::collectVariants, ContentManagerImpl::discoverContent, ContentManagerImpl::initialize, ContentManagerImpl::scanTypeFolder, PlaybackLoggerImpl::initialize, SystemManagerImpl::initializeSubsystems

### Community 18 - "Audio Playback"
Cohesion: 0.29
Nodes (7): AudioPlayer, AudioTools v1.1.3, ES8388 Audio Codec, ESP32-A1S Audio Kit v2.2, Internal I2C (GPIO32/33) for Codec, Library-First Approach, WAV Audio Format Support

### Community 19 - "Configuration"
Cohesion: 0.67
Nodes (3): ArduinoJson, ConfigLoader, Mode Configuration JSON Format

## Knowledge Gaps
- **221 isolated node(s):** `fix-headers.sh script`, `initialize`, `playFile`, `stop`, `isPlaying` (+216 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **10 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `SystemManagerImpl` connect `System Integration` to `Audio Playback`, `Content Discovery`, `Button Input`, `Configuration`, `System Integration`, `Audio Playback`, `Content Discovery`, `Audio Playback`, `Real-Time Clock`, `Button Input`, `Audio Playback`?**
  _High betweenness centrality (0.353) - this node is a cross-community bridge._
- **Why does `RtcManagerImpl` connect `Audio Playback` to `System Integration`, `Audio Playback`, `Button Input`?**
  _High betweenness centrality (0.141) - this node is a cross-community bridge._
- **Why does `PlaybackControllerImpl` connect `Configuration` to `Audio Playback`, `Button Input`, `System Integration`, `Audio Playback`, `Audio Playback`, `Audio Playback`?**
  _High betweenness centrality (0.124) - this node is a cross-community bridge._
- **What connects `fix-headers.sh script`, `initialize`, `playFile` to the rest of the system?**
  _243 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Audio Playback` be split into smaller, more focused modules?**
  _Cohesion score 0.05110336817653891 - nodes in this community are weakly interconnected._
- **Should `Content Discovery` be split into smaller, more focused modules?**
  _Cohesion score 0.09358974358974359 - nodes in this community are weakly interconnected._
- **Should `Button Input` be split into smaller, more focused modules?**
  _Cohesion score 0.06906906906906907 - nodes in this community are weakly interconnected._