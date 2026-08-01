# TODO — items flagged for Mlok's review

Autonomous implementation session (2026-08-01 onward, working `epics.md`'s
18 stories top to bottom on `dev` via one worktree+branch per story). This
file collects things that need a human decision, physical hardware access,
or verification this devcontainer cannot do — nothing here blocked the
session from continuing to the next story.

## Needs verification on a machine with Docker

- `backend/Dockerfile` + `backend/docker-compose.yml` (Story 1.3): no `docker`
  binary is available in this devcontainer, so the image build has never
  actually been run. Please `docker compose build` (from `backend/`) once
  before relying on it for deployment. The image should build fine —
  standard `node:24-bookworm-slim` + `npm ci` + native module — but it is
  genuinely unverified.

## Needs verification on real ESP32 hardware / a machine with the PlatformIO ESP32 toolchain

- All firmware stories (Epic 1: 1.1, 1.2, 1.4; Epic 2: 2.1, 2.2, 2.4, 2.5;
  Epic 4: 4.1): `firmware/platformio.ini`'s `[env:esp32dev]` target has never
  been built. This devcontainer has no ESP32 toolchain downloaded (that's a
  large download gated behind actually having hardware to flash) and no
  physical ESP32/sensors attached. What **is** verified: the pure-logic
  core of each story (unit conversion math, yaw correction, buffer
  ring behavior, etc.) via `pio test -e native` — see cerebrum.md's firmware
  test strategy entry for the split between tested pure logic and untested
  hardware-coupled code (ISR pulse counting, I2C reads, WiFi/HTTP, GPIO).
  Please build+flash+smoke-test each firmware story on real hardware before
  the regatta.

## Calibration constants — placeholder values, must be measured

- `firmware/src/correct/wind_speed.h`: `AnemometerCalibration.metersPerSecondPerHz`
  is a placeholder (`1.2`), not measured against the actual salvaged
  WH1080/WH1090 anemometer. Measure against a reference anemometer or the
  datasheet's rotation→speed constant before trusting readings.
- Magnetometer hard-iron/soft-iron calibration (Story 1.2 AC3): same
  principle — will land as a named, configurable constant, not computed yet
  since it needs the real installed magnetometer on the actual boat mount.

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
