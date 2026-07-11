#!/usr/bin/env bash
set -euo pipefail

echo "=== Initializing Python Virtual Environment ==="
if [ ! -d ".venv" ]; then
  python -m venv .venv || python3 -m venv .venv
  echo "Created virtual environment in .venv"
else
  echo ".venv already exists, skipping creation."
fi

# Locate the correct pip executable
if [ -f ".venv/Scripts/pip" ]; then
  PIP=".venv/Scripts/pip"
elif [ -f ".venv/bin/pip" ]; then
  PIP=".venv/bin/pip"
else
  PIP="pip"
fi

echo "=== Installing Python Dependencies ==="
if [ -f "requirements.txt" ]; then
  $PIP install -r requirements.txt
else
  echo "requirements.txt not found, skipping dependency installation."
fi

# Run the agent instruction sync if available
if [ -f "scripts/sync_agent_docs.py" ]; then
  echo "=== Syncing Agent Instructions ==="
  if [ -f ".venv/Scripts/python" ]; then
    .venv/Scripts/python scripts/sync_agent_docs.py
  elif [ -f ".venv/bin/python" ]; then
    .venv/bin/python scripts/sync_agent_docs.py
  else
    python scripts/sync_agent_docs.py || python3 scripts/sync_agent_docs.py
  fi
fi

echo "=== Setup Complete ==="
