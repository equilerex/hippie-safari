#!/usr/bin/env bash
set -euo pipefail

write_if_missing() {
  local file="$1"
  local dir
  dir="$(dirname "$file")"

  mkdir -p "$dir"

  if [[ -f "$file" ]]; then
    echo "skip: $file already exists"
    return
  fi

  cat > "$file"
  echo "created: $file"
}

mkdir -p .ai/memory .ai/plans .ai/prompts .ai/skills .github

write_if_missing "AGENTS.md" <<'CONTENT'
# Project Agent Instructions

## Project type

TODO: Angular / React / TypeScript / ESP32 / PlatformIO / mixed

## Commands

Install:
`pnpm install`

Dev:
`pnpm dev`

Test:
`pnpm test`

Typecheck:
`pnpm typecheck`

Build:
`pnpm build`

ESP32 build:
`pio run -e <env>`

ESP32 upload:
`pio run -e <env> -t upload`

## Working rules

- Start by identifying the relevant files and symbols.
- Use Serena symbolic tools before broad grep/read.
- Do not scan the whole repo unless explicitly asked.
- Do not rewrite unrelated files.
- Make minimal diffs.
- Before editing, state:
  - target files
  - target symbols
  - planned change
  - verification command
- After editing, run the narrowest useful verification first.
- Stop after 2 failed fix loops and explain the blocker.
- Do not change architecture, dependencies, build config, board config, pins, or storage model without stating why.

## Memory rules

- Read only relevant `.ai/memory/*` files.
- Store only confirmed facts.
- Do not store vague chat summaries.
- Update memory after non-trivial verified work.

## Output rules

- Prefer patches, diffs, or changed blocks.
- Avoid full-file dumps unless requested.
- Avoid long explanations unless needed for a decision.
CONTENT

write_if_missing "GEMINI.md" <<'CONTENT'
# Gemini Instructions

Follow `AGENTS.md`.

Do not duplicate or diverge from the canonical project instructions.
CONTENT

write_if_missing "CLAUDE.md" <<'CONTENT'
# Claude Instructions

Follow `AGENTS.md`.

Do not duplicate or diverge from the canonical project instructions.
CONTENT

write_if_missing ".github/copilot-instructions.md" <<'CONTENT'
Follow the repository instructions in `AGENTS.md`.

Prefer minimal diffs. Do not perform broad repo scans unless explicitly asked. Do not invent APIs, selectors, routes, components, board pins, or build commands.
CONTENT

write_if_missing ".ai/README.md" <<'CONTENT'
# AI Project Context

This folder contains reviewed project context for AI coding agents.

## Structure

- `memory/project-facts.md`: stable stack, commands, architecture, hardware facts
- `memory/decisions.md`: durable decisions and rejected alternatives
- `memory/known-failures.md`: confirmed bugs, causes, fixes, verification commands
- `memory/progress.md`: current state, next steps, blockers
- `plans/`: task plans
- `prompts/`: reusable prompts
- `skills/`: project-specific mini-guides

## Rules

- Store only confirmed facts.
- Keep entries short.
- Prefer dates.
- Do not store raw chat summaries.
- Do not store secrets.
CONTENT

write_if_missing ".ai/memory/project-facts.md" <<'CONTENT'
# Project Facts

## Stack

- Framework:
- Language:
- Package manager:
- Test runner:
- Build tool:
- Main app entry:

## Architecture

- State:
- Routing:
- Data fetching:
- Styling:
- Component pattern:

## ESP32 / hardware, if relevant

- Board:
- PlatformIO env:
- Important pins:
- Reserved pins:
- I2C SDA:
- I2C SCL:
- Audio/I2S:
- Display:
- LEDs:

## Commands verified

- Install:
- Dev:
- Build:
- Test:
- Typecheck:
- Upload:
CONTENT

write_if_missing ".ai/memory/decisions.md" <<'CONTENT'
# Decisions

## Template

### YYYY-MM-DD: <decision>

Decision:
- <what was chosen>

Reason:
- <why>

Rejected:
- <what was not chosen>

Impact:
- <what future agents must preserve>
CONTENT

write_if_missing ".ai/memory/known-failures.md" <<'CONTENT'
# Known Failures

## Template

### <short name>

Symptom:
- <error or behavior>

Cause:
- <root cause>

Fix:
- <fix>

Verified:
- `<command>`
CONTENT

write_if_missing ".ai/memory/progress.md" <<'CONTENT'
# Progress

## Template

### YYYY-MM-DD

Done:
- <finished work>

Verified:
- `<command>`

Next:
- <next sensible step>

Blocked:
- <blocker or none>
CONTENT

write_if_missing ".ai/prompts/implement-feature.md" <<'CONTENT'
You are working in this repo.

Task:
<task>

Rules:
- Read AGENTS.md first.
- Read only relevant `.ai/memory/*` files.
- Use Serena symbolic lookup before broad search.
- Before editing, report:
  - target files
  - target symbols
  - proposed change
  - verification command
- Make the smallest safe diff.
- Do not rewrite unrelated files.
- Stop after 2 failed test-fix loops and explain the blocker.
- Update memory only with confirmed durable facts.
CONTENT

write_if_missing ".ai/prompts/debug-bug.md" <<'CONTENT'
Debug this issue:

<issue>

Rules:
- Do not edit first.
- Reproduce or identify likely reproduction path.
- Check `.ai/memory/known-failures.md`.
- Use Serena to find relevant symbols/references.
- Give root-cause hypothesis before patching.
- Patch only the smallest relevant area.
- Verify with the narrowest command first.
- Stop after 2 failed fix loops and explain the blocker.
CONTENT

write_if_missing ".ai/prompts/review-diff.md" <<'CONTENT'
Review this diff.

Focus:
- Correctness
- Architecture fit
- Type safety
- Accessibility, if UI
- Test coverage
- Unnecessary scope expansion
- Risky generated/slop changes

Output:
- Blockers
- Should-fix
- Nice-to-have
- Suggested minimal patch, only if obvious

Do not rewrite the whole solution.
CONTENT

write_if_missing ".ai/prompts/esp32-hardware-change.md" <<'CONTENT'
ESP32 / PlatformIO task:

<task>

Rules:
- Read `.ai/memory/project-facts.md` first.
- Treat pin changes as high risk.
- Do not change board/env/library versions without stating why.
- Prefer a minimal test sketch before integrating into main firmware.
- Verify with `pio run -e <env>`.
- Update project facts if pin mappings or board assumptions change.
CONTENT

write_if_missing ".ai/skills/angular.md" <<'CONTENT'
# Angular Notes

Use this only for project-specific Angular conventions.

## Defaults

- Prefer typed forms.
- Prefer existing state/routing patterns.
- Do not introduce new state libraries without approval.
- Preserve accessibility semantics.
CONTENT

write_if_missing ".ai/skills/react.md" <<'CONTENT'
# React Notes

Use this only for project-specific React conventions.

## Defaults

- Prefer existing component and state patterns.
- Do not introduce new state libraries without approval.
- Avoid unnecessary abstraction.
- Preserve accessibility semantics.
CONTENT

write_if_missing ".ai/skills/esp32-platformio.md" <<'CONTENT'
# ESP32 / PlatformIO Notes

Use this only for project-specific hardware and firmware conventions.

## Defaults

- Treat pin mappings as high risk.
- Verify board/env before changing PlatformIO config.
- Prefer minimal test sketches for new hardware modules.
- Record confirmed pins and modules in `.ai/memory/project-facts.md`.
CONTENT

write_if_missing ".aiignore" <<'CONTENT'
# Frontend
node_modules/
dist/
build/
coverage/
storybook-static/
.angular/
.next/
.nuxt/
.vite/
.cache/
.tmp/
*.log
*.map
*.tsbuildinfo

# ESP32 / PlatformIO
.pio/
.vscode/ipch/
*.bin
*.elf

# Generated / noisy
*.generated.*
*.schema.json
*.snap

# Lockfiles: hide by default unless dependency task needs them
package-lock.json
pnpm-lock.yaml
yarn.lock
CONTENT

write_if_missing ".repomixignore" <<'CONTENT'
node_modules/
dist/
build/
coverage/
storybook-static/
.angular/
.next/
.nuxt/
.vite/
.cache/
.tmp/
.pio/
.vscode/ipch/
*.log
*.map
*.bin
*.elf
CONTENT

echo
echo "AI repo setup complete."
echo
echo "Suggested next steps:"
echo "  1. Edit AGENTS.md commands for this repo."
echo "  2. Fill .ai/memory/project-facts.md."
echo "  3. Commit the setup:"
echo "     git add AGENTS.md GEMINI.md CLAUDE.md .github/copilot-instructions.md .ai .aiignore .repomixignore setup-ai-repo.sh"
echo "     git commit -m \"Add AI coding repo setup\""

