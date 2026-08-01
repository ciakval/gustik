# STATUS — gustik

> Single source of truth for resuming work. Read this FIRST when starting a session.
> Update this file at the end of every work phase so the next `/clear` resumes in 1 read.
> Last updated: 2026-08-01

---

## ✅ Done

- **Planning complete**: PRD, Architecture Spine, and epics.md (5 epics, 18 stories, all 15 FRs covered) all finalized — see `_bmad-output/planning-artifacts/`.
- **Autonomous implementation phase**, integration branch `dev` (from `main`), one short-lived git-worktree+branch per story (`.worktrees/<branch>`, gitignored), TDD'd, merged to `dev` after tests pass, both branches pushed. Working `epics.md` top to bottom.
- **Epic 1 (walking skeleton) — complete.** Stories 1.1–1.6, all merged: firmware wind speed (1.1) + direction/yaw-correction (1.2), backend ingest endpoint (1.3), firmware transmit+reconnect (1.4), backend serve latest+WS (1.5), public dashboard (1.6). Backend: 22 tests. Firmware: 18 native tests.
- **Epic 2 (connectivity resilience/diagnostics) — complete.** Stories 2.1–2.5, all merged: firmware local flash buffer (2.1), firmware backfill-on-reconnect (2.2), backend `backfilled` flag + `history-changed` WS event (2.3), firmware disconnect LED (2.4), firmware RSSI logging (2.5). Backend: 27 tests. Firmware: 36 native tests.
- Devcontainer fixed to include `build-essential`+`python3` (needed for `better-sqlite3` native compile — was entirely missing).
- Firmware test strategy established (no ESP32 hardware/toolchain in this devcontainer): pure logic under `firmware/src/correct/` + `transmit/*.cpp` (top-level only) unit-tested via PlatformIO's `native` env; hardware-coupled code under `sense/`, `transmit/hw/`, `main.cpp` is written but genuinely untested. `[env:esp32dev]` declared for real hardware but not build-verified. Full rationale: `.wolf/cerebrum.md`.
- `TODO.md` (repo root) tracks everything needing human/physical verification — check it alongside this file.
- 11 real bugs found+fixed+logged this phase (`.wolf/buglog.json` bug-007 through bug-011, plus some auto-detected noise entries from an OpenWolf hook mixed in): `@fastify/websocket` registration-order gotcha, PlatformIO native test env config (`test_build_src`, `-DUNITY_INCLUDE_DOUBLE`), PlatformIO one-test-per-directory convention, `openDb()` missing directory creation (found via manual smoke test, not the `:memory:` test suite — see cerebrum.md's smoke-test learning), and a same-story catch of a buffer-data-loss bug in Story 2.1's `drainAll()` before it ever shipped (fixed in Story 2.2 as `peekAll()`/`clear()`).
- CLAUDE.md "Project state" section and this file updated to reflect real code (was previously "no application code yet").

---

## 🚀 Next phase

**Goal:** Epic 3 (historical graph) next: Story 3.1 (backend `GET /readings/history`), 3.2 (dashboard graph, Chart.js), 3.3 (dashboard resync on `history-changed`). Then Epic 4 (4.1 firmware WiFi provisioning from config file, 4.2 written manual). Epic 5 (5.1 power endurance, 5.2 waterproof enclosure) is physical/mechanical only — no code deliverable, intentionally not attempted, see `TODO.md`.

**Process per story** (established, keep following): `git worktree add .worktrees/<branch> -b epic<N>/story-<X.Y>-<slug>` off current `dev` HEAD → TDD (test-first, verify RED, implement, verify GREEN) → for backend stories, also do a real smoke test (`node src/index.js` + curl), not just the `:memory:` suite → commit → push story branch → merge `--no-ff` into `dev` → re-run full suite on `dev` post-merge → push `dev` → remove worktree → next story.

### Open decisions carried from PRD/architecture (deferred, not blocking)
- Magnetometer physical placement — explicit AC in epics.md Story 5.2, resolve there.
- Physical shore/mobile-hotspot switch need — explicit open note in epics.md Story 4.1.
- Real-world Wi-Fi range from shore at Nechanice — untested; Story 2.5's RSSI logging + Epic 3's history endpoint will make this measurable post-event.
- Local buffer capacity (4h target, implemented) and exact sampling interval (currently a 3000ms placeholder in `firmware/src/main.cpp`) — pending real flash capacity check on hardware.
- **HMC5883L sourcing**: confirm the actual purchased magnetometer module's chip before wiring a specific vendor library — `sense/magnetometer.cpp` currently talks QMC5883L registers directly (no vendor lib dependency), sidestepping this but still worth confirming against real hardware.
- Epic ordering (1→2→3→4→5) was this run's own inference from PRD's 3-tier priority statement, not literally specified — unchanged, still worth a skim per epics.md's own note.

---

## 📁 Active architecture

- **Stack:** Backend: Node.js 24 + Fastify 5.11.x + better-sqlite3 13.0.x + `@fastify/websocket` + `@fastify/static@10.1.2` (pinned above 8.x/9.x due to a high-severity path-traversal/auth-bypass advisory only fixed at 10.1.2), tested with built-in `node:test`. Firmware: C++/Arduino (ESP32 core 2.x) via PlatformIO, pure logic tested via `pio test -e native` (Unity, needs `test_build_src=yes` + `-DUNITY_INCLUDE_DOUBLE`). Dashboard: vanilla JS + `format.js` (pure, tested) + `dashboard.js` (DOM wiring, untested shell), no build step, no Chart.js yet (Story 3.2).
- **Key tables / modules:** SQLite `readings` table (pinned schema) in `backend/src/store/` (snake_case↔camelCase boundary lives only here). `backend/src/ingest` (write + backfilled-flag decision), `backend/src/serve` (read + WS + history-changed broadcast), `backend/src/health`, `backend/src/static` (dashboard). `firmware/src/sense` (hardware I/O, untested), `firmware/src/correct` (pure logic, tested), `firmware/src/transmit` (payload/connection-monitor/led-policy/rssi-latch/ring-buffer pure logic tested; `transmit/hw/` WiFi+HTTP+clock+flash hardware, untested), `firmware/src/main.cpp` (wiring).
- **Patterns:** One PlatformIO test directory per test case (`test/test_<name>/test_<name>.cpp`, own `main()`). Firmware pure logic (no Arduino.h) vs hardware-coupled code kept in separate files/dirs. Backend: every story gets a real smoke test in addition to injected `:memory:` tests (caught a real bug Story 1.6 that 22 passing tests missed).

---

## ⚠️ External blockers (don't block coding)

- No `docker` binary in this devcontainer — `backend/Dockerfile`/`docker-compose.yml` unverified.
- No ESP32 hardware or PlatformIO ESP32 toolchain download attempted — `firmware`'s `[env:esp32dev]` unverified, all hardware-coupled firmware code untested by construction.
- `INGEST_TOKEN` env var required for `backend` to start — not yet set anywhere durable (dev secret, not committed). Static WiFi/backend/token placeholders in `firmware/src/main.cpp` (`CHANGE_ME_*`) — Story 4.1 replaces with a real on-flash config file.

---

## 🔧 Useful commands

```bash
# backend
cd backend && npm install && npm test        # run backend test suite (node:test)
cd backend && INGEST_TOKEN=x npm start        # run the server locally

# firmware
uv tool install platformio                     # one-time, if pio isn't on PATH
cd firmware && ~/.local/bin/pio test -e native  # run firmware pure-logic tests (no hardware/toolchain needed)
cd firmware && ~/.local/bin/pio run -e esp32dev # NOT verified in this devcontainer - real hardware/toolchain only
```

---

## 📚 References (read IF needed)

- `.wolf/cerebrum.md` — User Preferences + Key Learnings + Decision Log
- `.wolf/anatomy.md` — token-efficient file index
- `.wolf/buglog.json` — known bugs + fixes
- `TODO.md` (repo root) — everything flagged for Mlok's human/physical review
