# Review: Stack Versions & Reality-Check — Gustik Architecture Spine

**Reviewed:** ARCHITECTURE-SPINE.md — "Stack" table and version-pinned claims
**Review date:** 2026-08-01
**Method:** Independent WebSearch verification of every claim, no reliance on training-data recall.

## Scope

Verified each row of the Stack table against live sources (npm, GitHub, official docs) as of 2026-08-01, plus reality-checked the ecosystem defaults each choice leans on (Fastify plugin compatibility, PlatformIO/ESP32 core support status, magnetometer chip identity, Docker native-module friction).

---

## Findings, most severe first

### 1. CRITICAL (hardware, not caught by version research) — "HMC5883L" modules sold today are overwhelmingly QMC5883L clones, incompatible with the named library

The spine pins `jarzebski/Arduino-HMC5883L` (or Adafruit equivalent) as the magnetometer library, treating it as "vyměnitelné bez architektonického dopadu" (swappable without architectural impact). That framing addresses *library* swap-ability but misses a more fundamental problem: **the genuine Honeywell HMC5883L chip has been discontinued**, and the GY-271/GY-273 breakout boards actually sold today (including on Czech marketplaces) overwhelmingly ship a QMC5883L (or newer HP5883) die under HMC5883L-branded silkscreen. These are not register-compatible clones — different I2C address (0x0D vs the HMC5883L's 0x1E), different register map, different init sequence. A library written against the genuine HMC5883L (like jarzebski's) will silently fail to detect or will misread a QMC5883L-based module.

- This is a very well-documented, recurring gotcha in the Arduino/ESP32 hobbyist community (Arduino Forum, esp8266.com, Mischianti.org all confirm it independently).
- **Impact for Gustik:** since sensors are salvaged/purchased hardware for a real deployment with a mid-August 2026 target, this is a concrete risk of dead-on-arrival I2C communication with the magnetometer, discovered only at bring-up time — not a paper cut.
- **Recommendation:** add an explicit early validation step (I2C bus scan to confirm device address before committing to a specific driver) and consider naming a QMC5883L-aware library (e.g. `dfrobot/DFRobot_QMC5883` or a dual-chip-detecting library) as the primary fallback rather than an afterthought, since it is now the *likelier* real-world case, not the edge case.

Sources: [GY-273 QMC5883L clone HMC5883L magnetometer — Mischianti](https://mischianti.org/gy-273-qmc5883l-clone-hmc5883l-magnetometer-for-arduino-esp8266-and-esp32/), [Problem with GY-271 compass module HMC5883L != QMC5883L — Arduino Forum](https://forum.arduino.cc/t/resolved-problem-with-gy-271-compass-module-hmc5883l-qmc5883l/498978), [Are all new GY-270 Board Magnetometers now fake?! — Arduino Forum](https://forum.arduino.cc/t/are-all-new-gy-270-board-magnetometers-now-fake/1379276)

---

### 2. MODERATE — better-sqlite3's Docker/native-compile story is more fragile than "13.0.x pinned" implies, especially off glibc/x86_64

better-sqlite3 13.0.2 is confirmed the current npm release and does ship prebuilt binaries for Node 24 (ABI 137/N-API), so the version pin itself is accurate and current. However the architecture doesn't note that this is a *recent* fix: the v11 line had no Node 24 prebuilds at all (forced source compile), and even after v12 added Node 24 support, musl/Alpine arm64 prebuilt binaries lagged for months behind the glibc ones. Two concrete gotchas that will bite in Docker if unaddressed:

- If the Dockerfile's base image is Alpine (musl) rather than Debian-slim (glibc), or if the deployment target is ARM (plausible for a small self-hosted box), there is real risk of falling back to source compilation, which then requires build tools (python3, make, g++) in the image and can fail if the container's egress is restricted (node-gyp falls back to fetching headers from `unofficial-builds.nodejs.org`, which can be blocked by network policy).
- The spine's Structural Seed shows a plain "Docker container" with no base-image or CPU-arch decision recorded — this should be made explicit (recommend `node:24-slim`, glibc, and confirm target CPU arch of the "Mlok" server) so the better-sqlite3 choice is actually low-friction in practice, not just in theory.

Sources: [better-sqlite3 npm](https://www.npmjs.com/package/better-sqlite3), [Provide prebuilt binary for Node 24 musl — WiseLibs/better-sqlite3#1382](https://github.com/WiseLibs/better-sqlite3/issues/1382), [Missing prebuild-install release binaries for Node 24/N-API 137 — WiseLibs/better-sqlite3#1384](https://github.com/WiseLibs/better-sqlite3/issues/1384), [Node 24 Native Module Docker Build Fails? Here's the Fix — SkillDham](https://skilldham.com/blog/node-24-native-module-docker-build-fails)

---

### 3. MINOR/CONFIRMATION — PlatformIO's stance on Arduino ESP32 core 3.x is unchanged and still unsettled; the 2.x pin is correct but the spine should name the fallback path

Independently confirmed: PlatformIO (official `platformio/platform-espressif32`) still has **no official support** for Arduino-ESP32 core 3.x as of 2026 — the maintainers are not merging community contributions toward it. The community fork `pioarduino/platform-espressif32` is the de facto way to get core 3.x on PlatformIO and is actively maintained (updated as recently as July 2026). For a plain ESP32-WROOM (not one of the newer C6/H2/P4 chips that *require* core 3.x), the stable 2.x branch via official PlatformIO remains a sound, low-risk choice — this part of the spine's research holds up.

- Gap: the spine doesn't name `pioarduino` as the fallback if the 2.x branch runs into a blocking issue (e.g. a needed library only ships for core 3.x, or Espressif eventually stops backporting fixes to 2.x). Worth a one-line addition to "Deferred" so a future session doesn't have to re-research this from scratch.

Sources: [Community Platformio support for Arduino core 3.x.x — espressif/arduino-esp32#10039](https://github.com/espressif/arduino-esp32/discussions/10039), [pioarduino/platform-espressif32 — GitHub](https://github.com/pioarduino/platform-espressif32), [Arduino ESP32 Core 3.0.0 Released, but PlatformIO Support Still in Question — Electronics-Lab](https://www.electronics-lab.com/arduino-esp32-core-3-0-0-released-but-platformio-support-still-in-question/)

---

### 4. MINOR — `@fastify/static` and `@fastify/websocket` are correctly compatible with Fastify 5, but the spine's unversioned naming invites a deprecated-package trap

Both plugins check out functionally:
- `@fastify/static` requires **>=8.x** for Fastify `^5.x` compatibility; current npm latest is 10.1.2. Confirmed fine.
- `@fastify/websocket` v11.x (built on `ws@8`) is the version line compatible with Fastify 5.

Neither row in the Stack table pins a version, which is fine in itself, but there is a real trap for whoever scaffolds the backend: `fastify-static` (unscoped, no `@fastify/` prefix) is a **deprecated legacy package name** that still resolves on npm and is *not* the same as `@fastify/static`. An implementer typing `npm i fastify-static` from muscle memory or an outdated tutorial would get an incompatible/deprecated package. Worth a one-line note in the spine or backend README pinning the scoped package names explicitly (`@fastify/static@^8`, `@fastify/websocket@^11`) to close that gap.

Sources: [@fastify/static — npm](https://www.npmjs.com/package/@fastify/static), [fastify-static is deprecated — nestjs/nest#9716](https://github.com/nestjs/nest/issues/9716), [@fastify/websocket — npm](https://www.npmjs.com/package/@fastify/websocket)

---

### 5. CONFIRMED, no action needed — Node.js 24, Fastify 5.11.x, Chart.js 4.5.x are accurate and current

- **Node.js 24**: confirmed Active LTS since 2025-10-28, maintained through ~April 2028. Correct choice for a project targeting deployment through at least mid-2026 and likely beyond. (Node 26 is the newer "Current" line but not LTS yet, so staying on 24 is right.)
- **Fastify 5.11.x**: confirmed 5.11.0 is npm-latest. Fastify 5's own minimum supported Node version is v20, so Node 24 comfortably satisfies it — no version-skew risk.
- **Chart.js 4.5.x**: confirmed 4.5.1 is npm-latest (released 2025-10-13), no breaking newer major in flight.
- **As a sanity check on the SQLite choice itself**: Node 24's built-in `node:sqlite` module was independently checked as a possible newer/better-fit alternative to `better-sqlite3`. It remains stability-level "active development"/"release candidate" (not stable) as of Node 24, and is explicitly documented as not intended to replace a full driver for anything beyond light/prototype use. This confirms `better-sqlite3` is still the right call for v1, not an oversight.

Sources: [Node.js 24 Becomes LTS — NodeSource](https://nodesource.com/blog/nodejs-24-becomes-lts), [Node.js EOL dates](https://endoflife.date/nodejs), [fastify — npm](https://www.npmjs.com/package/fastify), [chart.js — npm](https://www.npmjs.com/package/chart.js?activeTab=readme), [Chart.js v4.5.1 release](https://github.com/chartjs/Chart.js/releases/tag/v4.5.1), [stabilization of node:sqlite module — nodejs/node#57445](https://github.com/nodejs/node/issues/57445), [SQLite — Node.js docs](https://nodejs.org/api/sqlite.html)

---

## Overall verdict

The author's version research holds up well — every explicitly pinned version (Node 24, Fastify 5.11.x, better-sqlite3 13.0.x, Chart.js 4.5.x, PlatformIO/core 2.x) is independently confirmed current and mutually compatible as of 2026-08-01, and no named technology is deprecated or about to be. The one finding that actually matters for the mid-August 2026 deployment deadline is outside the "software version" frame entirely: the physical HMC5883L chip is very likely not what will show up in a real salvaged/purchased sensor module, and the named Arduino library will not talk to the QMC5883L variant that ships under the same silkscreen today — this should be treated as a bring-up risk to de-risk early, not a documentation nit.
