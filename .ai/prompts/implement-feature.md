You are working in this repo.

Task:
<task>

Rules:
- Read GEMINI.md or CLAUDE.md first.
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
