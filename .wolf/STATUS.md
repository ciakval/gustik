# STATUS — gustik

> Single source of truth for resuming work. Read this FIRST when starting a session.
> Update this file at the end of every work phase so the next `/clear` resumes in 1 read.
> Last updated: 2026-08-01

---

## ✅ Done

- **Planning complete**: PRD, Architecture Spine, and epics.md (5 epics, 18 stories, all 15 FRs covered) finalized — see `_bmad-output/planning-artifacts/`.
- **Autonomous implementation phase, Epics 1–4 (all 16 code-bearing stories) complete**, on integration branch `dev` (from `main`), one short-lived git-worktree+branch per story (`.worktrees/<branch>`, gitignored), TDD'd, merged after tests pass, both branches pushed. Backend: **43 tests** (`node:test`). Firmware: **43 native tests** (`pio test -e native`).
  - **Epic 1 (walking skeleton):** 1.1 wind speed, 1.2 wind direction/yaw-correction, 1.3 ingest endpoint, 1.4 transmit+reconnect, 1.5 serve latest+WS, 1.6 public dashboard.
  - **Epic 2 (connectivity resilience):** 2.1 local flash buffer, 2.2 backfill-on-reconnect, 2.3 backend `backfilled` flag + `history-changed` WS event, 2.4 disconnect LED, 2.5 RSSI logging.
  - **Epic 3 (historical graph):** 3.1 backend history endpoint, 3.2 dashboard graph (Chart.js vendored), 3.3 dashboard resync after backfill (also fixed a real dual-WebSocket reconnect gap, see buglog bug-016).
  - **Epic 4 (unattended provisioning):** 4.1 firmware WiFi config-file provisioning (replaces hardcoded credentials), 4.2 written operation manual (`/manual.html`, linked from the dashboard).
- **Epic 5 (power endurance + waterproof enclosure) intentionally not attempted** — physical/mechanical, no software deliverable. See `TODO.md`.
- Devcontainer fixed to include `build-essential`+`python3` (better-sqlite3 native compile) and `chromium` (real headless-browser dashboard smoke tests via puppeteer-core, which ships bundled with the global `openwolf` install).
- Firmware test strategy: pure logic (no Arduino.h) under `correct/`, `transmit/*.cpp` (top-level only), `config/*.cpp` is unit-tested via PlatformIO's `native` env; hardware-coupled code (`sense/`, `transmit/hw/`, `config/hw/`, `main.cpp`) is written but genuinely untested — no ESP32 hardware/toolchain in this devcontainer. `[env:esp32dev]` declared but not build-verified.
- 16 real bugs found+fixed+logged this phase (`.wolf/buglog.json` bug-007 through bug-016; a few auto-detected low-signal entries are mixed in between). Highlights: `@fastify/websocket` registration-order gotcha, PlatformIO native test env config, `openDb()` missing directory (caught by manual smoke test, not `:memory:` tests), a buffer-data-loss bug caught before it shipped (Story 2.1→2.2), a dual-WebSocket reconnect gap caught before it shipped (Story 3.2→3.3).
- `TODO.md` (repo root) tracks everything needing human/physical verification or a product decision — Docker build verification, real-hardware firmware verification, placeholder calibration constants, the mobile-hotspot-credentials-in-a-public-manual tension (Story 4.2), Epic 5.
- CLAUDE.md "Project state" and this file updated to reflect real code throughout (was "no application code yet" at session start).

---

## 🚀 Next phase

**All planned code work (Epics 1–4, 16 stories) is done.** What's left is either physical (Epic 5, Mlok-only) or verification/decisions already itemized in `TODO.md`:

1. **Docker build verification** — `docker compose build` from `backend/` on a machine with Docker.
2. **Real ESP32 hardware verification** — flash `[env:esp32dev]` and smoke-test every firmware story on actual hardware (this devcontainer never built it).
3. **Calibration** — anemometer `metersPerSecondPerHz` and magnetometer hard-iron offsets are placeholders, need measuring against real installed hardware.
4. **Decision needed**: mobile hotspot credentials — physical label on the Station vs. in the (public, no-auth) manual page. See TODO.md.
5. **Epic 5** — powerbank endurance test (Story 5.1) and waterproof enclosure/mounting (Story 5.2), both physical/mechanical, Mlok-only.
6. Skim epics.md's own "Poznámka k rozsahu" note once — epic ordering/breakdown was this run's own inference from the PRD's priority statement, not literally specified.

**If resuming further autonomous work:** there is no more epics.md backlog to work through. Any next quest would come from Mlok directly (bug reports against the real hardware, UX feedback on the dashboard, a Story 5.x follow-up once hardware exists, or new scope).

### Open decisions carried from PRD/architecture (deferred, unchanged)
- Magnetometer physical placement — explicit AC in epics.md Story 5.2.
- Physical shore/mobile-hotspot switch need — epics.md Story 4.1 left it out of scope; current firmware auto-selects by priority+scan (see `config/station_config.h`'s `selectNetworkIndex`).
- Real-world Wi-Fi range from shore at Nechanice — untested; RSSI logging (2.5) + history endpoint (3.1) make it measurable post-event.
- Local buffer capacity (4h target, implemented) and exact sampling interval (currently a 3000ms placeholder in `firmware/src/main.cpp`) — pending real flash capacity check on hardware.
- HMC5883L vs QMC5883L sourcing — `sense/magnetometer.cpp` talks QMC5883L registers directly (no vendor lib dependency), sidesteps the library-choice risk but the actual purchased part should still be confirmed.

---

## 📁 Active architecture

- **Stack:** Backend: Node.js 24 + Fastify 5.11.x + better-sqlite3 13.0.x + `@fastify/websocket` + `@fastify/static@10.1.2` (pinned above 8.x/9.x — high-severity path-traversal/auth-bypass advisory), tested with built-in `node:test`. Firmware: C++/Arduino (ESP32 core 2.x) via PlatformIO, pure logic tested via `pio test -e native` (Unity, needs `test_build_src=yes` + `-DUNITY_INCLUDE_DOUBLE`). Dashboard: vanilla JS, Chart.js 4.5.1 vendored (no CDN, no build step).
- **Key tables / modules:** SQLite `readings` table (pinned schema) in `backend/src/store/` (snake_case↔camelCase boundary lives only here). `backend/src/ingest` (write + backfilled-flag decision), `backend/src/serve` (read + WS + history-changed broadcast + history endpoint), `backend/src/health`, `backend/src/static` (dashboard: `index.html`/`dashboard.js`/`live-socket.js`/`history-chart.js`/`history-chart-data.js`/`format.js`/`manual.html`/`vendor/chart.umd.min.js`). `firmware/src/sense` (hardware I/O, untested), `firmware/src/correct` (pure logic, tested), `firmware/src/transmit` (payload/connection-monitor/led-policy/rssi-latch/ring-buffer/buffer-capacity pure logic tested; `transmit/hw/` WiFi+HTTP+clock+flash hardware, untested), `firmware/src/config` (station_config parser + network selector, tested; `config/hw/` LittleFS loader, untested), `firmware/data/config.example.txt` (WiFi/token template, real `config.txt` gitignored), `firmware/src/main.cpp` (wiring).
- **Patterns:** One PlatformIO test directory per test case (`test/test_<name>/test_<name>.cpp`, own `main()`). Firmware pure logic (no Arduino.h) vs hardware-coupled code kept in separate files/dirs. One shared WS connection on the dashboard (`live-socket.js`) instead of per-feature sockets. Backend: every story gets a real smoke test (curl) in addition to injected `:memory:` tests, and dashboard stories get a real headless-Chromium render check.

---

## ⚠️ External blockers (don't block coding)

- No `docker` binary in this devcontainer — `backend/Dockerfile`/`docker-compose.yml` unverified.
- No ESP32 hardware or PlatformIO ESP32 toolchain download attempted — `firmware`'s `[env:esp32dev]` unverified, all hardware-coupled firmware code untested by construction.
- `INGEST_TOKEN` env var required for `backend` to start — not yet set anywhere durable (dev secret, not committed). Real WiFi/backend/token config lives in `firmware/data/config.txt` (gitignored, not yet created — copy `config.example.txt`).

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
cd firmware && ~/.local/bin/pio run -t uploadfs -e esp32dev # upload data/config.txt to device LittleFS (Story 4.1)

# real headless-browser dashboard check (see cerebrum.md)
# needs: sudo apt-get install chromium  (already in .devcontainer/Dockerfile)
```

---

## 📚 References (read IF needed)

- `.wolf/cerebrum.md` — User Preferences + Key Learnings + Decision Log
- `.wolf/anatomy.md` — token-efficient file index
- `.wolf/buglog.json` — known bugs + fixes
- `TODO.md` (repo root) — everything flagged for Mlok's human/physical review
