# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# OpenWolf

@.wolf/OPENWOLF.md

This project uses OpenWolf for context management. Read and follow `.wolf/OPENWOLF.md` every session. Check `.wolf/cerebrum.md` before generating code. Check `.wolf/anatomy.md` before reading files.

## Project state

All 16 code-bearing stories from `_bmad-output/planning-artifacts/epics.md` (Epics 1-4: walking skeleton, connectivity resilience, historical graph, unattended provisioning) are implemented and merged to `dev` (one short-lived branch + git worktree per story, TDD'd, merged after tests pass — see `.worktrees/` convention, gitignored). `main` stays at the last human-reviewed point; `dev` has the full implementation. Epic 5 (power endurance + waterproof enclosure) is physical/mechanical only, intentionally not attempted — see `TODO.md`. Two subprojects:

- **`backend/`** — Node.js 24 + Fastify 5.11.x + better-sqlite3 13.0.x + `@fastify/websocket` + `@fastify/static`. `npm install && npm test` (43 tests, built-in `node:test`, zero extra assertion deps — see cerebrum.md). `npm start` runs the server (needs `INGEST_TOKEN` env var). Serves the dashboard (`src/static/`: live view, historical Chart.js graph, operation manual) as static files, no frontend build step. `Dockerfile`/`docker-compose.yml` exist but are **not build-verified** — no `docker` binary in this devcontainer, see `TODO.md`.
- **`firmware/`** — PlatformIO project, ESP32 Arduino core 2.x (`[env:esp32dev]`, real hardware target — **not build-verified**, no ESP32 toolchain/hardware in this devcontainer). Hardware-coupled code (ISR pulse counting, I2C, WiFi/HTTP, GPIO, LittleFS config/buffer) lives under `sense/`, `transmit/hw/`, `config/hw/`, `main.cpp` and is untested here. Pure algorithmic logic (unit conversions, yaw correction, buffer ring math, config parsing) lives under `correct/`, `transmit/*.cpp`, `config/*.cpp` (all Arduino.h-free) and IS unit-tested via `pio test -e native` (43 tests, PlatformIO's host-native env — lightweight, no ESP32 download). Install PlatformIO with `uv tool install platformio` if not already on PATH.

See `.wolf/STATUS.md` for full story-by-story detail and `TODO.md` for everything flagged for human review (Docker/real-hardware verification, placeholder calibration constants, a mobile-hotspot-credentials product decision, Epic 5's physical-only stories).

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
- `docs/` — project knowledge base referenced by BMAD (`project_knowledge` in config.toml); also holds `docs/superpowers/specs/` — design docs from the Superpowers workflow, used for post-v1 changes outside BMAD's epic/story scope (see "Choosing a workflow" below).
- `.claude/commands/` — custom slash commands: `reframe` (UI framework selection, mirrors the OpenWolf `reframe` skill) and `security-audit` (layered security audit workflow).
- `.claude/skills/` — the full BMAD agent/workflow skill set (analyst, PM, architect, dev, UX designer, tech writer personas plus supporting workflows like brainstorming, PRD, architecture, story creation).
- `.devcontainer/` — Debian trixie-slim container with Node.js, Claude Code CLI, `openwolf` CLI, GitHub CLI, and `uv` preinstalled; see below.

## Working with BMAD

This project uses the BMAD method for planning (product brief → PRD → architecture → epics/stories → dev). Relevant skills are prefixed `bmad-*` (e.g. `bmad-product-brief`, `bmad-prd`, `bmad-architecture`, `bmad-create-epics-and-stories`, `bmad-dev-story`) and agent personas are invoked by name (Mary=Analyst, John=PM, Winston=Architect, Sally=UX, Amelia=Dev, Paige=Tech Writer — see `_bmad/config.toml` for the full roster). The product brief exists; next BMAD steps per its standard flow would be PRD and architecture docs, not yet created.

## Choosing a workflow for new work: BMAD vs Superpowers

The BMAD flow above shipped v1: product brief → `epics.md` → 16 stories → implemented and merged to `dev` (see "Project state"). That backlog is exhausted — per `.wolf/STATUS.md`, the only work still planned from it is Epic 5 (physical/mechanical, Mlok-only) and the verification items in `TODO.md`. Everything else from here on is post-launch: bug reports, small feature or UX requests, ad-hoc fixes against the deployed app — not new epics.

For that kind of scoped, single-concern change, default to the **Superpowers** workflow instead of spinning up BMAD ceremony: `superpowers:brainstorming` (clarify + design) → a spec written to `docs/superpowers/specs/YYYY-MM-DD-<topic>-design.md` → `superpowers:writing-plans` → implementation via TDD (`superpowers:test-driven-development`). This is lighter-weight than BMAD's epic/story machinery and fits a change that touches a handful of files without needing a PRD or architecture-doc update.

Reserve BMAD (`bmad-create-epics-and-stories`, `bmad-dev-story`, the agent personas) for work that's genuinely epic-shaped: a new Story 5.x once physical hardware exists to test against, or anything large/novel enough to need multi-story breakdown or changes to the PRD/architecture docs. If a change only needs `epics.md` read for context, not written to, it's a Superpowers change, not a BMAD one.

Either flow touches the same repo, so OpenWolf's session bookkeeping (`STATUS.md`, `anatomy.md`, `cerebrum.md`, `buglog.json`, `memory.md`) is mandatory regardless of which one produced the change — those rules are orthogonal to the planning method and apply on top of both.

Artifact locations:
- BMAD: `_bmad-output/planning-artifacts/` (brief, `epics.md`); `_bmad-output/implementation-artifacts/` (stories, if the BMAD flow is used again).
- Superpowers: `docs/superpowers/specs/`.

## Dev environment

The devcontainer (`.devcontainer/Dockerfile`) provides Node.js, the Claude Code CLI, GitHub CLI (`gh`), and `uv` (Python package manager) — `uv` is present for future firmware tooling but nothing Python-based exists in the repo yet. `.devcontainer/post-create.sh` runs on container creation and just prints tool versions and checks SSH-agent/GH token forwarding; it performs no project setup since there is no project to set up yet.
