# Graph Report - .  (2026-07-05)

## Corpus Check
- Corpus is ~31,881 words - fits in a single context window. You may not need a graph.

## Summary
- 345 nodes · 557 edges · 17 communities (15 shown, 2 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 33 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Module 0|Module 0]]
- [[_COMMUNITY_Button Input|Button Input]]
- [[_COMMUNITY_Button Input|Button Input]]
- [[_COMMUNITY_Configuration|Configuration]]
- [[_COMMUNITY_Content Discovery|Content Discovery]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Module 8|Module 8]]
- [[_COMMUNITY_Playback Control|Playback Control]]
- [[_COMMUNITY_Playback Control|Playback Control]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Audio Playback|Audio Playback]]
- [[_COMMUNITY_Playback Control|Playback Control]]
- [[_COMMUNITY_Playback Control|Playback Control]]
- [[_COMMUNITY_Module 15|Module 15]]
- [[_COMMUNITY_Module 16|Module 16]]

## God Nodes (most connected - your core abstractions)
1. `SystemManagerImpl` - 34 edges
2. `PlaybackLoggerImpl` - 32 edges
3. `AudioPlayerImpl` - 28 edges
4. `PlaybackControllerImpl` - 28 edges
5. `PlaybackLogger` - 23 edges
6. `ContentManagerImpl` - 23 edges
7. `ButtonManagerImpl` - 19 edges
8. `ContentManager` - 17 edges
9. `TimeWindow` - 15 edges
10. `PlaybackController` - 14 edges

## Surprising Connections (you probably didn't know these)
- `AudioPlayerImpl` --inherits--> `IAudioPlayer`  [EXTRACTED]
  src/AudioPlayerImpl.h → include/AudioPlayer.h
- `ButtonManagerImpl` --inherits--> `ButtonManager`  [EXTRACTED]
  src/ButtonManagerImpl.h → include/ButtonManager.h
- `loadTypeConfig` --references--> `ModeConfig`  [EXTRACTED]
  src/ConfigLoaderImpl.h → include/Config.h
- `loadModeFolderConfig` --references--> `ModeConfig`  [EXTRACTED]
  src/ContentManagerImpl.h → include/Config.h
- `PlaybackControllerImpl` --references--> `PlaybackState`  [EXTRACTED]
  src/PlaybackControllerImpl.h → include/Config.h

## Import Cycles
- None detected.

## Communities (17 total, 2 thin omitted)

### Community 0 - "Module 0"
Cohesion: 0.09
Nodes (38): EventType, LogEntry, details, eventType, modeIndex, timestamp, typeIndex, variantIndex (+30 more)

### Community 1 - "Button Input"
Cohesion: 0.08
Nodes (30): ButtonManager, getButtonPressed, getLastError, getLastEvent, initialize, isTypeAvailable, poll, SystemState (+22 more)

### Community 2 - "Button Input"
Cohesion: 0.08
Nodes (29): ButtonState, ContentManager, discoverContent, getLastError, getType, getTypeCount, getVariantCount, getVariantPath (+21 more)

### Community 3 - "Configuration"
Cohesion: 0.09
Nodes (28): ConfigLoader, getLastError, isTimeWindowActive, loadTypeConfig, selectModeForTime, time_t, vector, PlaybackControllerImpl (+20 more)

### Community 4 - "Content Discovery"
Cohesion: 0.13
Nodes (27): TypeContent, defaultMode, folderName, modes, typeIndex, variants, ContentManagerImpl, cachedTypes (+19 more)

### Community 5 - "Audio Playback"
Cohesion: 0.11
Nodes (24): AudioBoardStream, EncodedAudioStream, AudioPlayerImpl, audioKit, copier, currentFile, currentVolume, decoded (+16 more)

### Community 6 - "Audio Playback"
Cohesion: 0.16
Nodes (9): string, vector, ModeConfig, modeName, timeWindows, VariantIndex, currentIndex, totalVariants (+1 more)

### Community 7 - "Audio Playback"
Cohesion: 0.08
Nodes (26): AudioStreamState, bytesRead, currentFilePath, isOpen, isPlaying, totalBytes, ContentScan, errorMessage (+18 more)

### Community 8 - "Module 8"
Cohesion: 0.12
Nodes (22): TimeWindow, absoluteEndTime, absoluteStartTime, dayMask, endHour, endMin, priority, startHour (+14 more)

### Community 9 - "Playback Control"
Cohesion: 0.11
Nodes (18): PlaybackLogger, flushQueue, getLastError, getQueueSize, initialize, logConfigError, logEvent, logModeChanged (+10 more)

### Community 10 - "Playback Control"
Cohesion: 0.15
Nodes (12): PlaybackController, enterStandby, exitStandby, getLastError, getVariantIndex, handleButtonEvent, initialize, isInStandby (+4 more)

### Community 11 - "Audio Playback"
Cohesion: 0.15
Nodes (12): SystemManager, getAudioPlayer, getButtonManager, getConfigLoader, getContentManager, getLastError, getPlaybackController, getPlaybackLogger (+4 more)

### Community 12 - "Audio Playback"
Cohesion: 0.17
Nodes (11): IAudioPlayer, getLastError, getVolume, initialize, isHealthy, isPlaying, playFile, process (+3 more)

### Community 13 - "Playback Control"
Cohesion: 0.25
Nodes (8): time_t, PlaybackState, currentModeIndex, currentTypeIndex, currentVariantIndex, isPlaying, lastInteractionTime, playStartTimeMs

### Community 14 - "Playback Control"
Cohesion: 0.33
Nodes (6): PlaybackRequest, action, requestTimeMs, typeIndex, variantIndex, PlaybackAction

## Knowledge Gaps
- **166 isolated node(s):** `fix-headers.sh script`, `initialize`, `playFile`, `stop`, `isPlaying` (+161 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `SystemManagerImpl` connect `Button Input` to `Module 0`, `Button Input`, `Configuration`, `Content Discovery`, `Audio Playback`, `Audio Playback`, `Module 8`, `Playback Control`, `Audio Playback`, `Audio Playback`?**
  _High betweenness centrality (0.361) - this node is a cross-community bridge._
- **Why does `PlaybackControllerImpl` connect `Configuration` to `Button Input`, `Button Input`, `Audio Playback`, `Playback Control`, `Playback Control`, `Playback Control`?**
  _High betweenness centrality (0.188) - this node is a cross-community bridge._
- **Why does `AudioPlayerImpl` connect `Audio Playback` to `Button Input`, `Audio Playback`, `Audio Playback`?**
  _High betweenness centrality (0.141) - this node is a cross-community bridge._
- **What connects `fix-headers.sh script`, `initialize`, `playFile` to the rest of the system?**
  _166 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Module 0` be split into smaller, more focused modules?**
  _Cohesion score 0.09358974358974359 - nodes in this community are weakly interconnected._
- **Should `Button Input` be split into smaller, more focused modules?**
  _Cohesion score 0.08333333333333333 - nodes in this community are weakly interconnected._
- **Should `Button Input` be split into smaller, more focused modules?**
  _Cohesion score 0.08064516129032258 - nodes in this community are weakly interconnected._