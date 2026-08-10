# TODO — items flagged for Mlok's review

Autonomous implementation session (2026-08-01 onward, working `epics.md`'s
18 stories top to bottom on `dev` via one worktree+branch per story). This
file collects things that need a human decision, physical hardware access,
or verification this devcontainer cannot do — nothing here blocked the
session from continuing to the next story.

## CI pipeline (`.github/workflows/ci.yml`) — backend deploy now wired and live

Lint+build+test run on every push/PR (backend: eslint + `node:test` +
`docker build`; firmware: `pio check` + `pio test -e native` + a real
`pio run -e esp32dev` build, see below). Firmware gets packaged as a
90-day build artifact on every push, and as a GitHub Release on version
tags (`v*`).

Backend deploy to `bombur.remesh.cz` (real server, `plachtis` account,
`DEPLOY_DIR=/home/plachtis/DOCKER/gustik`) is real, configured, and
**live** (`DEPLOY_ENABLED=true`): repo secrets `VPS_SSH_KEY` /
`INGEST_TOKEN`, repo variables `VPS_HOST` / `VPS_USER` / `VPS_SSH_PORT` /
`DEPLOY_DIR` are all set. `build-backend-image` builds `backend/Dockerfile`
on every push/PR and, on a push to `main`, pushes it to GHCR
(`ghcr.io/ciakval/gustik-backend`, tagged `:latest` and `:<sha>`) - the
image running in production is the exact same one CI build-verified, not
a separate rebuild. `deploy-backend` then only ships `backend/compose.yaml`
to the server (rsync - reinstalled on the server by hand after briefly
being unavailable, see buglog bug-025/bug-026), writes `.env` fresh from
the `INGEST_TOKEN` secret, logs the server into GHCR with this run's own
short-lived `GITHUB_TOKEN` (re-authenticates fresh every deploy, nothing
stored permanently), then `docker compose pull && up -d` and a scoped
`docker image prune` (filtered to this project's label - an unscoped
prune was tried once by hand during setup and reclaimed 1.5GB of *other*
projects' dangling images on the shared host, which is out of this
project's bounds). Container is named `gustik`, joins the pre-existing
external `proxy` Docker network, and is reverse-proxied by the host's
Caddy instance (`gustik.remesh.cz → gustik:3000`) - a separate,
human-managed compose project this workflow never touches.

`backend/Dockerfile` is build-verified by CI on every push (previously
completely untested — no `docker` binary in this devcontainer even now).

## Firmware `[env:esp32dev]` — now build-verified, still needs real hardware

`pio run -e esp32dev` was assumed hardware-gated and never actually tried
until the CI pipeline task ran it for real — turns out it only needed
internet access (ESP32 toolchain download), which was available the whole
time. It builds clean now (both in this devcontainer and continuously in
CI) — **but two things still need real hardware, which no devcontainer or
CI runner has:**

- **Flash+smoke-test on an actual ESP32** before the regatta — the build
  succeeding proves the code compiles for the real target, not that any
  hardware-coupled code (ISR pulse counting, I2C reads, WiFi/HTTP, GPIO,
  LittleFS) actually works on real silicon. Grab a build artifact from
  the CI run (or a tagged release) and flash it, or just `pio run -t
  upload -e esp32dev` from a machine with the device attached.
- **Flash usage is tight: 90.6% (1.19MB of 1.31MB)** at the current code
  size, per the first real CI build. There's no OTA mechanism (station is
  flashed by hand once), so the default partition table's two-OTA-slot
  layout wastes space that isn't needed — a custom single-app partition
  table would recover meaningful headroom if this becomes a real
  constraint as more code is added. Not implemented (out of scope for
  the CI task that surfaced it) - see cerebrum.md Decision Log.

Also: the ESP32 Arduino core version ended up as 3.x, not the "stable
2.x" the architecture spine originally called for — the registry no
longer serves any 2.x release at all. See cerebrum.md Decision Log/buglog
bug-017 for the full correction.

## Calibration constants — placeholder values, must be measured

- `firmware/src/correct/wind_speed.h`: `AnemometerCalibration.metersPerSecondPerHz`
  is a placeholder (`1.2`), not measured against the actual salvaged
  WH1080/WH1090 anemometer. Measure against a reference anemometer or the
  datasheet's rotation→speed constant before trusting readings.
- Magnetometer hard-iron/soft-iron calibration (Story 1.2 AC3): same
  principle — will land as a named, configurable constant, not computed yet
  since it needs the real installed magnetometer on the actual boat mount.

## Product/security decision needed: mobile hotspot credentials in the manual (Story 4.2)

- Story 4.2's AC2 says the manual should state the exact SSID/password of
  the backup mobile hotspot so a non-technical rozhodčí can type it into
  their phone. But `backend/src/static/manual.html` is served with **no
  auth** (matches the dashboard's own public, no-login design, FR-9) —
  publishing the hotspot's real password there means anyone with the URL
  can read it, which defeats the point of it being a password.
  `manual.html` was written with a placeholder ("see the label on the
  Station") instead of a real credential, and flags this tension inline.
  **Needs a decision from Mlok**: (a) accept it as-is and put the real
  SSID/password on a physical label on the Station enclosure instead
  (Story 5.2's mechanical work), (b) accept the weak "URL isn't
  advertised" security-through-obscurity and put the real password in the
  manual anyway, or (c) something else. Neither the PRD nor architecture
  spine resolved this explicitly — NFR-8 only gates the write endpoint,
  read endpoints (dashboard, and now the manual) are consciously public.

## Open questions carried from PRD/architecture (unchanged, still open)

See `.wolf/STATUS.md` "Open decisions carried from PRD/architecture" for the
full list (magnetometer physical placement, shore/hotspot physical switch,
real-world Wi-Fi range, buffer capacity / sampling interval exact values,
HMC5883L vs QMC5883L sourcing confirmation).

## Epic 5 — not attempted (physical/mechanical, no code deliverable)

- Story 5.1 (8h+ powerbank endurance test) and Story 5.2 (waterproof
  enclosure + sensor mounting) require the actual assembled hardware on a
  real powerbank / real enclosure. There is no software artifact for these
  — they're physical construction and testing tasks for Mlok, not something
  an autonomous coding session can do or fake. Left untouched.
