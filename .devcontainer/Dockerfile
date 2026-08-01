FROM debian:trixie-slim

ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=1000

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        gnupg \
        git \
        less \
        openssh-client \
        sudo \
        vim \
    && rm -rf /var/lib/apt/lists/*

# GitHub CLI
RUN curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg -o /usr/share/keyrings/githubcli-archive-keyring.gpg \
    && chmod go+r /usr/share/keyrings/githubcli-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" \
        > /etc/apt/sources.list.d/github-cli.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends gh \
    && rm -rf /var/lib/apt/lists/*

# Node.js, needed only to install/run the Claude Code CLI (an npm package)
RUN curl -fsSL https://deb.nodesource.com/setup_lts.x | bash - \
    && apt-get install -y --no-install-recommends nodejs \
    && rm -rf /var/lib/apt/lists/*

RUN npm install -g @anthropic-ai/claude-code
RUN npm install -g openwolf

# Non-root user. UID/GID match the host user by default so bind-mounted
# host files (~/.claude, ~/.gitconfig) stay writable/readable without chown.
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m -s /bin/bash $USERNAME \
    && echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Persist bash history across container rebuilds. /commandhistory is backed
# by a named volume (see devcontainer.json), so it survives even though the
# rest of the home directory is rebuilt from scratch each time. history -a
# flushes after every command instead of only on a clean shell exit.
RUN mkdir -p /commandhistory \
    && chown $USER_UID:$USER_GID /commandhistory \
    && echo "export HISTFILE=/commandhistory/.bash_history" >> /home/$USERNAME/.bashrc \
    && echo "export HISTSIZE=10000 HISTFILESIZE=20000" >> /home/$USERNAME/.bashrc \
    && echo 'PROMPT_COMMAND="history -a; ${PROMPT_COMMAND:-}"' >> /home/$USERNAME/.bashrc

# Trust GitHub's host key up front so SSH-based git works non-interactively
RUN mkdir -p /etc/ssh && ssh-keyscan -H github.com >> /etc/ssh/ssh_known_hosts 2>/dev/null

USER $USERNAME

# uv, a fast Python package/dependency manager
ENV PATH="/home/$USERNAME/.local/bin:${PATH}"
RUN curl -LsSf https://astral.sh/uv/install.sh | sh
