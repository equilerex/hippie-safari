# AI Project Context

Do not read the whole `.ai/` folder by default. this folder contains reviewed context for coding agents.
Load files from here only when relevant.

## Memory

Use `.ai/memory/` for durable, verified project facts only.

- `memory/project-facts.md`
  Stable stack, commands, architecture shape, hardware facts, board/pin assumptions.

- `memory/decisions.md`
  Durable decisions, rejected alternatives, and constraints future work must preserve.

- `memory/known-failures.md`
  Confirmed failures with symptom, cause, fix, and verification command.

- `memory/progress.md`
  Current state, next step, and blockers when useful across sessions.
 
## Tooling

Use Serena for codebase navigation, editing, and context:
- Use Serena symbolic lookup (MCP tools) for finding definitions, references, and symbols before performing raw grep scans.
- Refer to Serena's local memories folder at `.serena/memories/` (including `cpp_module_summaries.md` containing class-level functional descriptions) to understand system components.

Use Graphify for codebase structure and documentation relationship indexing:
- Use Graphify before broad searches for architecture, dependencies, or change impact; verify results in source.
- Query the graph using `graphify query "<question>"` (CLI) or `query_graph` (MCP) to trace dependencies, symbols, and call flows.

## AI Workflow Experiments
- Refer to the [AI Workflow Experiments Guide](docs/ai-workflow-experiments.md) to understand the local LLM, Graphify, and Serena architecture layers implemented in this repository.

## Embedded Safety

**Interrupt handlers (ISR):** Code must be atomic and minimal. Defer work to tasks/event queues. No blocking calls, no malloc, no I2C inside ISR.

**Shared state:** Flag access to variables modified in ISR without volatile keyword or mutex protection. Check `.ai/memory/known-failures.md` for I2C contention patterns.

**Hardware timing:** Document timing assumptions (I2C clock, ISR latency windows, delay() calls) near code. Verify on hardware before committing.

**Testing:** Hardware integration tests only—no unit mocks for GPIO/I2C. Real board verification required for peripheral code.

**Code review focus:** Register access (volatile), ISR re-entrancy, bus contention (mutex use), no blocking in ISR, memory limits (ESP32 ~320KB free RAM after OS).

**Known constraints:** Reference `.ai/memory/project-facts.md` (pin map, board revisions) and `.ai/memory/known-failures.md` (silicon bugs, timing gotchas) before implementing similar code.

## Update rules

Update `.ai/memory/` only when a durable, verified fact changes.

Update memory after:

- build/test command changes
- architecture or module-boundary changes
- public API/data-contract changes
- dependency/tooling assumptions change
- board/pin/module facts change
- repeated failure gets a verified fix
- durable decision is made

Do not update memory for:

- routine edits
- temporary debugging
- raw logs
- speculation
- chat summaries
- one-off experiments
- unverified assumptions

## LLM-Specific Customizations
- Prefer using MCP tools (`query_graph`, `get_node`, etc.) when available rather than raw CLI commands or broad text grep searches.
- Ensure terminal commands are proposed clearly before running.
- Follow PlatformIO ESP32 upload/monitor instructions strictly when building or deploying firmware.
