# Graph Report - .  (2026-07-02)

## Corpus Check
- Corpus is ~11,838 words - fits in a single context window. You may not need a graph.

## Summary
- 24 nodes · 23 edges · 5 communities (4 shown, 1 thin omitted)
- Extraction: 87% EXTRACTED · 13% INFERRED · 0% AMBIGUOUS · INFERRED: 3 edges (avg confidence: 0.88)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Community 0|Community 0]]
- [[_COMMUNITY_Community 1|Community 1]]
- [[_COMMUNITY_Community 2|Community 2]]
- [[_COMMUNITY_Community 3|Community 3]]
- [[_COMMUNITY_Community 4|Community 4]]

## God Nodes (most connected - your core abstractions)
1. `Hippy Safari` - 5 edges
2. `Observation Station` - 4 edges
3. `Content Variants` - 4 edges
4. `Content Discovery` - 4 edges
5. `ESP32-A1S Audio Kit` - 3 edges
6. `Design Principles` - 3 edges
7. `Species` - 2 edges
8. `Intro` - 2 edges
9. `Easter Eggs` - 2 edges
10. `Audio Signal Path` - 2 edges

## Surprising Connections (you probably didn't know these)
- `SD-Card Content Storage` --semantically_similar_to--> `Content Discovery`  [INFERRED] [semantically similar]
  docs/physical-project-description.md → docs/technical-project-descriptio.md
- `Hippy Safari` --conceptually_related_to--> `Observation Station`  [EXTRACTED]
  docs/technical-project-descriptio.md → docs/physical-project-description.md
- `Audio Signal Path` --conceptually_related_to--> `Speaker Configuration`  [INFERRED]
  docs/technical-project-descriptio.md → docs/physical-project-description.md
- `Content Flexibility` --rationale_for--> `Content Discovery`  [INFERRED]
  docs/physical-project-description.md → docs/technical-project-descriptio.md
- `ESP32-A1S Audio Kit` --conceptually_related_to--> `Audio Signal Path`  [EXTRACTED]
  docs/physical-project-description.md → docs/technical-project-descriptio.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Content Management System** — docs_technical_project_descriptio_species, docs_technical_project_descriptio_intro, docs_technical_project_descriptio_eastereggs, docs_technical_project_descriptio_contentvariants [EXTRACTED 1.00]
- **Audio Processing Stack** — docs_physical_project_description_esp32a1s, docs_technical_project_descriptio_audiopath, docs_physical_project_description_speakerconfiguration, docs_physical_project_description_audiocodec [EXTRACTED 1.00]
- **Time Management System** — docs_technical_project_descriptio_rtc, docs_technical_project_descriptio_networktime, docs_technical_project_descriptio_utctimestamp [EXTRACTED 1.00]
- **Physical Installation** — docs_physical_project_description_observationstation, docs_physical_project_description_buttonled, docs_physical_project_description_speakerconfiguration, docs_physical_project_description_weatherprotection [EXTRACTED 1.00]

## Communities (5 total, 1 thin omitted)

### Community 0 - "Community 0"
Cohesion: 0.29
Nodes (7): Audio Codec, Button LEDs, ESP32-A1S Audio Kit, Observation Station, Speaker Configuration, Weather Protection, Audio Signal Path

### Community 1 - "Community 1"
Cohesion: 0.29
Nodes (7): Content Flexibility, SD-Card Content Storage, Content Discovery, Content Directory Structure, Design Principles, Engineering Philosophy, Graceful Fallback Behavior

### Community 2 - "Community 2"
Cohesion: 0.47
Nodes (6): Configuration Approach, Content Variants, Easter Eggs, Hippy Safari, Intro, Species

### Community 3 - "Community 3"
Cohesion: 0.67
Nodes (3): Network Time, RTC (Real Time Clock), UTC Timestamp Format

## Knowledge Gaps
- **7 isolated node(s):** `Network Time`, `Button LEDs`, `Speaker Configuration`, `SD-Card Content Storage`, `Audio Codec` (+2 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Hippy Safari` connect `Community 2` to `Community 0`, `Community 1`?**
  _High betweenness centrality (0.476) - this node is a cross-community bridge._
- **Why does `Observation Station` connect `Community 0` to `Community 2`?**
  _High betweenness centrality (0.344) - this node is a cross-community bridge._
- **Why does `Engineering Philosophy` connect `Community 1` to `Community 2`?**
  _High betweenness centrality (0.308) - this node is a cross-community bridge._
- **Are the 2 inferred relationships involving `Content Discovery` (e.g. with `Content Flexibility` and `SD-Card Content Storage`) actually correct?**
  _`Content Discovery` has 2 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Network Time`, `Graceful Fallback Behavior`, `Button LEDs` to the rest of the system?**
  _12 weakly-connected nodes found - possible documentation gaps or missing edges._