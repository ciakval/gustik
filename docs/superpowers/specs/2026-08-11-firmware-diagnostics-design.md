# Firmware boot/connectivity diagnostics — design note

2026-08-11. Post-launch, scoped change (per CLAUDE.md's Superpowers-vs-BMAD
guidance) — no epics.md/PRD impact. **Note:** the `superpowers` skill plugin
is not installed in this session/environment (only its prior output docs
exist under `docs/superpowers/`), so this note stands in for a formal
`superpowers:brainstorming` session — decisions below were confirmed directly
with Mlok via a clarifying question, not derived unilaterally.

## Problem

Verifying the real ESP32 hardware can connect to Wi-Fi (first hardware
bring-up test, `TODO.md` item "Real ESP32 hardware verification") was
blocked: `firmware/src/` has zero `Serial` output anywhere, and the one
existing indicator (disconnect LED, GPIO2) reflects sender health — WiFi
*and* backend reachability combined — so it can't isolate a WiFi-only
failure from a backend-unreachable one.

## Decision

Add permanent (not throwaway) diagnostics, confirmed with Mlok:

1. **Serial** (`Serial.begin(115200)`, matches existing `monitor_speed`):
   boot banner (network count from config, backend URL — never the token),
   then one compact line per sample cycle (WiFi status/IP/RSSI, send
   result, clock-sync state, buffer count).
2. **Two new diagnostic LEDs**, on unused GPIOs on the same physical board
   side as the existing sensor wiring (GPIO27 anemometer, GPIO34 vane —
   the side with the EN/RST button on a standard 30-pin ESP32-WROOM
   DevKitC), so all diagnostic wiring is physically grouped:
   - GPIO25 — **config-loaded**: on once `StationConfig` parsed ≥1
     network entry (proves `config.txt` was actually uploaded/parsed,
     independent of whether any network is in range).
   - GPIO26 — **WiFi-connected**: mirrors `WiFi.status() == WL_CONNECTED`
     each cycle, isolated from backend reachability.
   - Existing GPIO2 disconnect LED unchanged (send/backend health).

   Both are plain output-capable GPIOs, not strapping pins, not shared
   with I2C (21/22 default) or UART0 (1/3, now needed for Serial).

This turns hardware bring-up into a 3-LED decision tree: config LED off →
`config.txt` never landed on LittleFS; config on but WiFi LED off → no
configured SSID in range or bad password; WiFi on but disconnect LED
lit → backend unreachable/misconfigured.

## Also fixed (blocking bug found while setting this up)

`firmware/platformio.ini`'s `[env:esp32dev]` never set
`board_build.filesystem = littlefs`. `firmware/src/config/hw/config_loader.cpp`
uses `LittleFS.begin()`/`LittleFS.open()`, but PlatformIO's `uploadfs`
target defaults to a SPIFFS image absent that setting — `config.txt` would
have uploaded successfully but in the wrong filesystem format, so
`LittleFS.open("/config.txt")` would silently fail to find it on boot.
This would have broken Story 4.1's entire config-file provisioning path on
real hardware, undetected until now (native tests can't catch it — no
filesystem involved). Fixed alongside this change since it directly
blocks the WiFi bring-up test.

## Addendum (same session): magnetometer wiring, real chip mismatch

Once WiFi+backend were confirmed working end-to-end, Mlok asked which pins
to wire the magnetometer to and pointed at `scripts/` (a bench-side Python
I2C driver, added separately in commit `d901ee9`, never previously
cross-checked against the firmware) for lessons learned characterizing the
real sensor with a USB-I2C bridge. That surfaced a real mismatch (bug-029):
firmware targeted QMC5883L (I2C `0x0D`) on an unconfirmed architecture-time
assumption; the real chip is QMC5883P (I2C `0x2C`), a different register
map entirely. `firmware/src/sense/magnetometer.cpp`/`.h` rewritten to match
(mirrors `scripts/src/gustik_scripts/qmc5883p.py`), `firmware/src/main.cpp`
now carries the real bench-measured hard-iron offsets from
`scripts/qmc5883p-calibration.json` instead of `0.0`/`0.0` placeholders,
range set to 8G to match. The bench testing also confirmed the real mount's
handedness (`up=-z`, `forward=+x` — chip mounted upside down), which
requires negating the raw Y axis; done in `magnetometer.cpp` (hardware
layer) rather than `correct/wind_direction.cpp` (kept generic/pure/tested,
unchanged) — see the comments in both files for the exact sign-convention
derivation. Wiring answer: I2C default pins, `SDA→GPIO21`, `SCL→GPIO22`
(ESP32 Arduino core default via bare `Wire.begin()`, unclaimed by any other
pin in this firmware), `VCC→3.3V`, `GND→GND`.

Not yet verified against real silicon — the magnetometer isn't physically
wired to the ESP32 yet, only build-verified + cross-checked against the
bench driver's register map. Also: the calibration describes the bench
assembly, not the final boat-mounted enclosure — `scripts/README.md` is
explicit that a calibration is only valid for one rigid assembly, so
Story 5.2 (physical mounting) should prompt a recalibration.

## Not done / explicitly out of scope

- No pure-function extraction for the two new LEDs' logic — both are a
  direct one-line `digitalWrite(pin, condition ? HIGH : LOW)`, same as
  existing untested `main.cpp` wiring; not worth a new testable module
  (existing `shouldLedSignalDisconnect()` earned its own tested function
  because it encodes a real policy decision — no debounce — these don't).
- No change to `led_policy.h`/disconnect-LED semantics.
- No attempt to test Serial/LED/filesystem code under `pio test -e native`
  — all of it is hardware-coupled by construction (Arduino.h/WiFi.h/
  LittleFS.h), consistent with the project's existing native/esp32dev
  test-split convention.
