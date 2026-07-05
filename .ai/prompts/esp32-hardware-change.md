ESP32 / PlatformIO task:

<task>

Rules:
- Read `.ai/memory/project-facts.md` first.
- Treat pin changes as high risk.
- Do not change board/env/library versions without stating why.
- Prefer a minimal test sketch before integrating into main firmware.
- Verify with `pio run -e <env>`.
- Update project facts if pin mappings or board assumptions change.
