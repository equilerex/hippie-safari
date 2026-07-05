# AI Project Context

This folder contains reviewed context for coding agents.

Load files from here only when relevant. Do not read the whole `.ai/` folder by default.

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

## Prompts

Use `.ai/prompts/` for reusable task workflows.

- `prompts/implement-feature.md`
  Feature implementation workflow.

- `prompts/debug-bug.md`
  Debugging workflow.

- `prompts/review-diff.md`
  Code review workflow.
 
## Tooling

Use Serena for codebase navigation and editing:

- definitions
- references
- symbols
- call relationships
- targeted edits

Use Context7 for external library/framework documentation:

- current APIs
- version-specific examples
- configuration details
- migration-sensitive behavior

Do not use Context7 for ordinary local project code.

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

If unsure, propose the memory update as a patch instead of applying it.