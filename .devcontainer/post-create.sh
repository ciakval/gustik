#!/usr/bin/env bash
set -euo pipefail

#git config --global --add safe.directory "$(pwd)"

# Alias the host's home path inside the container so absolute paths written
# into the bind-mounted ~/.claude (e.g. by Claude Code's plugin manager)
# resolve regardless of which side — host or container — wrote them. See
# devcontainer.json's HOST_HOME comment for the full explanation.
if [ -n "${HOST_HOME:-}" ] && [ "$HOST_HOME" != "$HOME" ] && [ ! -e "$HOST_HOME" ]; then
    sudo ln -s "$HOME" "$HOST_HOME"
    echo "linked $HOST_HOME -> $HOME (host/container home alias)"
fi

echo "node:   $(node --version)"
echo "claude: $(claude --version 2>/dev/null || echo 'not found')"
echo "gh:     $(gh --version | head -n1)"
echo "uv:     $(uv --version 2>/dev/null || echo 'not found')"

if ssh-add -l >/dev/null 2>&1; then
    echo "ssh agent: forwarded, $(ssh-add -l | wc -l) key(s) loaded"
elif [ -n "${GH_TOKEN:-}${GITHUB_TOKEN:-}" ]; then
    echo "ssh agent: not forwarded, but GH_TOKEN/GITHUB_TOKEN is set"
    gh auth setup-git || true
else
    echo "ssh agent: not forwarded and no GH_TOKEN set."
    echo "  -> run 'ssh-add -l' on the host before opening the container, or export GH_TOKEN."
fi
