# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# OpenWolf

@.wolf/OPENWOLF.md

This project uses OpenWolf for context management. Read and follow `.wolf/OPENWOLF.md` every session. Check `.wolf/cerebrum.md` before generating code. Check `.wolf/anatomy.md` before reading files.

## Project state

**Gustik has no application code yet.** The repository currently contains only planning artifacts (BMAD) and dev-environment tooling (OpenWolf, devcontainer). There is no package.json, build system, linter, or test suite to run — do not invent commands for these. When implementation starts (firmware and/or web dashboard), this section and the OpenWolf files (`.wolf/STATUS.md`, `.wolf/anatomy.md`) should be updated to reflect the real stack and commands.

## What Gustik is

Gustik is a DIY weather station for scout sailing regattas (Czech scouting, dinghy classes P550/Optimist/Topaz). It measures wind speed and direction on the water, at the committee/start boat — not on shore, since wind on small lakes differs meaningfully between shore and water. Organizers use this to judge safety (P550s can capsize in strong wind) and to brief crews before each race.

Planned architecture (from the product brief, not yet built):
- **Sensor unit:** ESP32-WROOM reading anemometer/wind-vane sensors salvaged from a WH1080/WH1090 weather station, plus an HMC5883L magnetometer to correct wind direction for the anchored boat's yaw (it can rotate on its mooring).
- **Connectivity:** Wi-Fi (boat hotspot or shore signal, 100–200 m) pushing data to a web backend; best-effort local buffering on the ESP32 with backfill on reconnect, plus an on-device LED to signal lost connection.
- **Dashboard:** web app showing live wind speed/direction and a time-history graph, usable from mobile and PC, for organizers and troop leaders (same view for both — no separate roles in v1).
- **Power:** USB-C powerbank, targeting a full race day per charge.
- Scope is deliberately single-station, single-regatta for v1 (target: deployed to a real regatta by mid-August 2026). Multi-station and multi-regatta aggregation are explicitly out of scope, noted as future vision.

Full detail lives in `_bmad-output/planning-artifacts/briefs/brief-gustik-2026-08-01/brief.md` (product brief) and `addendum.md` (team roles, comparable-project research, sensor/magnetometer background). Both are in Czech; `_bmad/config.toml` sets `document_output_language = "Czech"` for BMAD-generated docs project-wide.

## Repository layout

- `.wolf/` — OpenWolf state: `STATUS.md` (current quest / next steps, read first), `anatomy.md` (file index with token costs, check before reading files), `cerebrum.md` (learned preferences/conventions/decisions), `buglog.json` (known bugs+fixes), `memory.md` (session log).
- `_bmad/` — BMAD method installation (agents, workflows, config). `_bmad/config.toml` is installer-managed/read-only; durable overrides go in `_bmad/custom/`.
- `_bmad-output/planning-artifacts/` — BMAD-generated planning docs (currently: the product brief). `_bmad-output/implementation-artifacts/` is where epics/stories will land once planning moves into implementation.
- `docs/` — project knowledge base referenced by BMAD (`project_knowledge` in config.toml); currently empty.
- `.claude/commands/` — custom slash commands: `reframe` (UI framework selection, mirrors the OpenWolf `reframe` skill) and `security-audit` (layered security audit workflow).
- `.claude/skills/` — the full BMAD agent/workflow skill set (analyst, PM, architect, dev, UX designer, tech writer personas plus supporting workflows like brainstorming, PRD, architecture, story creation).
- `.devcontainer/` — Debian trixie-slim container with Node.js, Claude Code CLI, `openwolf` CLI, GitHub CLI, and `uv` preinstalled; see below.

## Working with BMAD

This project uses the BMAD method for planning (product brief → PRD → architecture → epics/stories → dev). Relevant skills are prefixed `bmad-*` (e.g. `bmad-product-brief`, `bmad-prd`, `bmad-architecture`, `bmad-create-epics-and-stories`, `bmad-dev-story`) and agent personas are invoked by name (Mary=Analyst, John=PM, Winston=Architect, Sally=UX, Amelia=Dev, Paige=Tech Writer — see `_bmad/config.toml` for the full roster). The product brief exists; next BMAD steps per its standard flow would be PRD and architecture docs, not yet created.

## Dev environment

The devcontainer (`.devcontainer/Dockerfile`) provides Node.js, the Claude Code CLI, GitHub CLI (`gh`), and `uv` (Python package manager) — `uv` is present for future firmware tooling but nothing Python-based exists in the repo yet. `.devcontainer/post-create.sh` runs on container creation and just prints tool versions and checks SSH-agent/GH token forwarding; it performs no project setup since there is no project to set up yet.
