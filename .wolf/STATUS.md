# STATUS — gustik

> Single source of truth for resuming work. Read this FIRST when starting a session.
> Update this file at the end of every work phase so the next `/clear` resumes in 1 read.
> Last updated: 2026-08-11

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
- **[2026-08-09] First post-launch change, done via Superpowers (not a new BMAD story)**: `/readings` ingest now accepts timezone-aware (`Z` or explicit `±HH:MM` offset) and naive (assumed Europe/Prague local time) `capturedAt` timestamps, normalizing all of them to canonical UTC before storage so the history query's lexicographic sort (`backend/src/store/readings.js`) keeps working unmodified. New `backend/src/ingest/timestamp.js` (`normalizeCapturedAt`, 8 unit tests) does the conversion; `backend/src/ingest/routes.js` wires it in. Dashboard history chart now displays times in Europe/Prague (new `backend/src/static/timezone.js`) instead of hardcoded UTC — verified with a real headless-Chromium screenshot showing the correct local tick. Backend test count: 43 → 55. See `docs/superpowers/specs/2026-08-09-timestamp-timezone-support-design.md` and `docs/superpowers/plans/2026-08-09-timestamp-timezone-support.md`. CLAUDE.md's new "Choosing a workflow for new work" section documents why this used Superpowers instead of BMAD.
- **[2026-08-10] Backend deployment wired end-to-end and live-verified at https://gustik.remesh.cz.** Real server: `bombur.remesh.cz:10022`, user `plachtis`, `DEPLOY_DIR=/home/plachtis/DOCKER/gustik`. Repo secrets (`VPS_SSH_KEY`, `INGEST_TOKEN`) and variables (`VPS_HOST`, `VPS_USER`, `VPS_SSH_PORT`, `DEPLOY_DIR`) all set by Mlok. `backend/docker-compose.yml`: `container_name: gustik`, joins pre-existing external Docker network `proxy`, app stays on its default port 3000 (no `PORT` override) — Mlok updated the host's Caddyfile (`/home/ciakval/DOCKER/web/Caddyfile`, a separate human-managed compose project, never touched by CI) to `reverse_proxy gustik:3000`, replacing a placeholder nginx container that previously occupied that name/network slot (removed by hand as part of this work). `.github/workflows/ci.yml` `deploy-backend` job rewired to the real secret/variable names and ships `backend/` via tar-over-SSH instead of rsync (server has no `rsync` binary and no sudo to install one — verified with `sudo -n`); writes `.env` fresh every deploy from the `INGEST_TOKEN` secret (no more hand-maintained server file); health check now curls the public `https://gustik.remesh.cz/health` from the runner instead of an SSH-local `localhost:3000` check (nothing is published to the host anymore). Manually dry-ran the full flow once by hand (tar ship → write .env → `docker compose up -d --build` → prune → health check) before enabling anything — confirmed dashboard, `/health`, and ingest auth (401 without/with-wrong token) all work through the real Caddy+TLS path. Two bugs caught during that dry run, both fixed, see buglog bug-025/bug-026. `DEPLOY_ENABLED` repo variable flipped to `true` and a real automated CI deploy run completed successfully (push `201be22`, run 31426222564) — `.env` on the server now holds the real `INGEST_TOKEN` secret (confirmed different from the dry-run placeholder), health check passed against the public URL. Auto-deploy on every push to `main` is now live. Note: confirming the real token landed required printing it once to a terminal to compare against the dry-run placeholder — it's visible in that session's transcript, worth being aware of even though it's not committed anywhere. Then reverted the sync step from tar back to rsync (Mlok installed rsync on the server by hand) — re-verified rsync works and leaves `.env` alone.
- **[2026-08-11] First real ESP32 hardware bring-up, WiFi connectivity confirmed on the physical device.** Done via Superpowers-style scoped change (no `superpowers` skill plugin installed in this environment - a plain design note stood in, see `docs/superpowers/specs/2026-08-11-firmware-diagnostics-design.md`). Added permanent Serial diagnostics (`Serial.begin(115200)`, one status line per sample cycle: WiFi/IP/RSSI, send result, clock sync, buffer count) and 2 new diagnostic LEDs (GPIO25 config-loaded, GPIO26 WiFi-connected - grouped on the EN/RST-button board side with existing sensor pins 27/34) to `firmware/src/main.cpp`, since the firmware previously had zero observability (no Serial output anywhere, and the one existing LED conflates WiFi failure with backend-unreachable failure). Caught and fixed a real bug in the process (bug-028): `firmware/platformio.ini`'s `[env:esp32dev]` never set `board_build.filesystem = littlefs`, so `pio run -t uploadfs` would have silently built a SPIFFS image instead of LittleFS - `config_loader.cpp`'s `LittleFS.open("/config.txt")` would never have found the uploaded config on real hardware, breaking Story 4.1's provisioning path undetected (native tests can't catch it, no filesystem involved). Flashed real hardware from this machine (PlatformIO CLI at `~/.platformio/penv/bin/pio`, device on `/dev/ttyUSB0`): `pio run -e esp32dev` builds clean (91.9% flash, up slightly from the new diagnostics code), `uploadfs` + `upload` both succeeded, and the serial monitor confirmed the device associated to a real WiFi network (got a DHCP IP, RSSI -25 to -36 dBm) and synced NTP time - the NTP success is the stronger proof since it requires real upstream internet reachability, not just local AP association. `firmware/data/config.txt` (real WiFi credentials, gitignored) filled in directly by Mlok, never pasted into chat. `backend.*` fields in that config are still placeholders - full send-to-backend path not yet exercised on hardware.
- **[2026-08-10] GHCR-based deploy + `compose.yaml` rename — implemented on branch `chore/ghcr-backend-deploy` (worktree `.worktrees/chore-ghcr-backend-deploy`), PR pending, not yet merged to `main`.** Addresses two things Mlok flagged: (1) the pipeline was building the Docker image twice (once in CI just to verify, once again on the server) with no guarantee the server ran the same bytes CI tested; (2) `docker-compose.yml` should be `compose.yaml`, the Compose v2-preferred filename. Changes: `backend/docker-compose.yml` → `backend/compose.yaml` (git mv), now has both `image: ghcr.io/ciakval/gustik-backend:latest` (what's actually deployed) and `build: .` (kept for local dev, `docker compose up --build`). CI's `docker-build-backend` job renamed to `build-backend-image`, now uses `docker/login-action@v3` + `docker/build-push-action@v6` (GHCR auth via the run's own `GITHUB_TOKEN`, job-level `permissions: packages: write`) to push `:latest` + `:<sha>` tags on pushes to `main` only (PRs/other branches still build-verify, just don't push — no reason to publish an image for code that isn't deploying). `deploy-backend` simplified: now syncs only `compose.yaml` (not the whole `backend/` tree — nothing to build on the server anymore), adds a "Log in to GHCR on the server" step (pipes the ephemeral `GITHUB_TOKEN` via stdin/here-string, same discipline as the `.env`-write step — never a CLI arg), then `docker compose pull && up -d` instead of `--build`. GHCR image kept **private** (repo is public, but private-by-default + fresh ephemeral login every deploy needs zero manual GHCR package-visibility setup and is the safer default). **Not yet end-to-end tested for real** — `build-backend-image` only pushes on `main`, so this branch's own CI run build-verifies but can't push/pull; the real GHCR push→pull→deploy path only gets exercised once this merges. Validated everything short of that: YAML + `actionlint` clean, 55/55 backend tests pass, manually re-confirmed rsync still works against the server and leaves `.env` alone.

---

## 🚀 Next phase

**All planned code work (Epics 1–4, 16 stories) is done.** What's left is either physical (Epic 5, Mlok-only) or verification/decisions already itemized in `TODO.md`:

1. ~~Docker build verification~~ / ~~backend deployment~~ — **done 2026-08-10**: backend is built and running for real on `bombur.remesh.cz`, live at https://gustik.remesh.cz, with `DEPLOY_ENABLED=true` — every push to `main` now auto-deploys via CI.
2. **Real ESP32 hardware verification** — **WiFi connectivity confirmed 2026-08-11** (see Done log above). Still open: full send-to-backend path (config.txt `backend.*` fields still placeholder), magnetometer/vane/anemometer sensor smoke-test on real wiring, disconnect-LED/backfill behavior under a real outage, and a re-run of every other firmware story's happy path on actual hardware.
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

- No `docker` binary in this devcontainer — `backend/Dockerfile`/`backend/compose.yaml` can't be build-verified locally (CI's `build-backend-image` job does this instead, on every push).
- No ESP32 hardware in the original devcontainer — still true there, `[env:esp32dev]` still can't be built/flashed from inside it. **Superseded outside the devcontainer 2026-08-11**: real hardware bring-up (build/uploadfs/upload/monitor, WiFi connect confirmed) was done directly from Mlok's own machine, PlatformIO CLI at `~/.platformio/penv/bin/pio`, device on `/dev/ttyUSB0`.
- `INGEST_TOKEN` env var required for `backend` to start — not yet set anywhere durable (dev secret, not committed). Real WiFi/backend/token config lives in `firmware/data/config.txt` (gitignored) — **exists now on Mlok's machine as of 2026-08-11** (real WiFi creds filled in, `backend.*` still placeholder) — still not present in the devcontainer.

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
