#!/usr/bin/env bash
set -euo pipefail

#git config --global --add safe.directory "$(pwd)"

echo "node:   $(node --version)"
echo "claude: $(claude --version 2>/dev/null || echo 'not found')"
echo "gh:     $(gh --version | head -n1)"

if ssh-add -l >/dev/null 2>&1; then
    echo "ssh agent: forwarded, $(ssh-add -l | wc -l) key(s) loaded"
elif [ -n "${GH_TOKEN:-}${GITHUB_TOKEN:-}" ]; then
    echo "ssh agent: not forwarded, but GH_TOKEN/GITHUB_TOKEN is set"
    gh auth setup-git || true
else
    echo "ssh agent: not forwarded and no GH_TOKEN set."
    echo "  -> run 'ssh-add -l' on the host before opening the container, or export GH_TOKEN."
fi
