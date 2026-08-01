# STATUS — gustik

> Single source of truth for resuming work. Read this FIRST when starting a session.
> Update this file at the end of every work phase so the next `/clear` resumes in 1 read.
> Last updated: 2026-08-01

---

## ✅ Done

- **Planning complete**: PRD, Architecture Spine, and epics.md (5 epics, 18 stories, all 15 FRs covered) all finalized — see git history before this implementation phase for details, or `_bmad-output/planning-artifacts/`.
- **Autonomous implementation phase started** on integration branch `dev` (branched from `main`), one short-lived git-worktree+branch per story (`.worktrees/<branch>`, gitignored), merged to `dev` after tests pass, `dev` pushed after every merge. Story branches also pushed before merge. Working `epics.md` top to bottom, Epic 1 first.
- **Story 1.3** (`epic1/story-1.3-ingest-endpoint`, merged): founded `backend/` (Fastify 5.11.x + better-sqlite3 13.0.x). `POST /readings` (bearer auth, idempotent on `client_id`, accepts array 1..N) + `GET /health`. 11 tests (`node:test`).
- **Story 1.5** (`epic1/story-1.5-serve-latest`, merged): `GET /readings/latest` + `GET /readings/live` WS channel (`@fastify/websocket`), identical wire shape (AD-9). 14 tests total.
- **Story 1.1** (`epic1/story-1.1-wind-speed`, merged): founded `firmware/` (PlatformIO, ESP32 Arduino core 2.x). Anemometer pulse counting (hardware, untested) + pulses→m/s pure conversion (tested). 3 native tests.
- **Story 1.2** (`epic1/story-1.2-wind-direction`, merged): vane + magnetometer sense layer (hardware, untested) + yaw-correction pure logic (tested, verified stable under simulated fixed-true-wind scenario across all 8 yaw octants). 9 native tests total.
- Devcontainer fixed to include `build-essential`+`python3` (needed for `better-sqlite3` native compile — was entirely missing, real gap not just a nice-to-have).
- Firmware test strategy established (no ESP32 hardware/toolchain in this devcontainer): pure logic under `firmware/src/correct/` unit-tested via PlatformIO's `native` env; hardware-coupled code under `sense/`/`transmit/`(hw)/`main.cpp` is written but genuinely untested. `[env:esp32dev]` declared for real hardware but not build-verified. Full rationale: `.wolf/cerebrum.md` Key Learnings.
- `TODO.md` created at repo root — tracks everything needing human/physical verification (Docker build, real-hardware firmware flash, placeholder calibration constants, Epic 5's physical-only stories). Check it alongside this file.
- 4 real bugs found+fixed+logged this phase (see `.wolf/buglog.json` bug-007, bug-008): `@fastify/websocket` registration-order gotcha; PlatformIO native test env needing `test_build_src=yes` + `-DUNITY_INCLUDE_DOUBLE`; PlatformIO one-test-per-directory convention.

---

## 🚀 Next phase

**Goal:** Continue Epic 1 top to bottom: Story 1.4 (firmware transmit with auto-reconnect) next, then 1.6 (dashboard), completing the walking skeleton. Then Epic 2 (2.1→2.5), Epic 3 (3.1→3.3), Epic 4 (4.1→4.2). Epic 5 (5.1, 5.2) is physical/mechanical only — no code deliverable, intentionally not attempted, see `TODO.md`.

**Process per story** (established, keep following): `git worktree add .worktrees/<branch> -b epic<N>/story-<X.Y>-<slug>` off current `dev` HEAD → TDD (test-first, verify RED, implement, verify GREEN) → commit → push story branch → merge `--no-ff` into `dev` on the main checkout → re-run full test suite on `dev` post-merge → push `dev` → remove worktree → move to next story.

### Open decisions carried from PRD/architecture (deferred, not blocking)
- Magnetometer physical placement — explicit AC in epics.md Story 5.2, resolve there, don't re-open now.
- Physical shore/mobile-hotspot switch need — explicit open note in epics.md Story 4.1.
- Real-world Wi-Fi range from shore at Nechanice — untested, FR-6 RSSI logging (Story 2.5, not yet implemented) will make this measurable post-event.
- Local buffer capacity (4h target) and exact sampling interval (~2–5s, currently a 3000ms placeholder in `firmware/src/main.cpp`) — pending real flash capacity check, relevant at Story 2.1.
- **HMC5883L sourcing**: confirm the actual purchased magnetometer module's chip before wiring a specific vendor library — Story 1.2's `sense/magnetometer.cpp` currently talks QMC5883L registers directly (no vendor lib dependency yet), which sidesteps this but should still be confirmed against real hardware.
- Epic 1/2/3/4 exact epic ordering was this run's own inference from PRD's 3-tier priority statement, not literally specified — flagged for Mlok to skim once (unchanged from before, see epics.md's own "Poznámka k rozsahu").

---

## 📁 Active architecture

- **Stack:** Backend: Node.js 24 + Fastify 5.11.x + better-sqlite3 13.0.x + `@fastify/websocket`, tested with built-in `node:test` (zero extra assertion deps). Firmware: C++/Arduino (ESP32 core 2.x) via PlatformIO, pure logic tested via `pio test -e native` (Unity framework, needs `test_build_src=yes` + `-DUNITY_INCLUDE_DOUBLE` — see buglog bug-008). Dashboard: not started (Story 1.6).
- **Key tables / modules:** SQLite `readings` table (pinned schema, see architecture spine) in `backend/src/store/`. `backend/src/ingest` (write), `backend/src/serve` (read+WS), `backend/src/health`. `firmware/src/sense` (hardware I/O, untested), `firmware/src/correct` (pure logic, tested), `firmware/src/main.cpp` (wiring).
- **Patterns:** snake_case↔camelCase conversion lives only in `backend/src/store`. Firmware pure logic (no Arduino.h) vs hardware-coupled code kept in separate files/dirs so the pure half stays testable. One PlatformIO test directory per test case (`test/test_<name>/test_<name>.cpp`, own `main()`) — never two test files sharing a directory.

---

## ⚠️ External blockers (don't block coding)

- No `docker` binary in this devcontainer — `backend/Dockerfile`/`docker-compose.yml` unverified.
- No ESP32 hardware or PlatformIO ESP32 toolchain download attempted — `firmware`'s `[env:esp32dev]` unverified, and all hardware-coupled firmware code (ISR, I2C, WiFi/HTTP, GPIO) is untested by construction.
- `INGEST_TOKEN` env var required for `backend` to start — not yet set anywhere durable (dev secret, not committed).

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

- `.wolf/cerebrum.md` — User Preferences + Do-Not-Repeat + Decision Log
- `.wolf/anatomy.md` — token-efficient file index
- `.wolf/buglog.json` — known bugs + fixes
- `TODO.md` (repo root) — everything flagged for Mlok's human/physical review
