# 02 — Crate inventory

Mapped against the *actual* modules in `firmware/src/`, not against a generic ESP32 project.
Versions checked on crates.io / docs.rs on **2026-08-14**. "Route B" = `no_std`/esp-hal (**chosen**),
"Route A" = `std`/ESP-IDF (fallback only).

> **Revised 2026-08-14** after Mlok's answers: HTTPS is `reqwless` + `embedded-tls` with verification
> off (no git deps), and the magnetometer uses the `qmc5883p` crate rather than a hand-written driver.
> Everything on this page is now available from crates.io under normal semver.

## Foundation

| Need | Route B crate | Version | Notes |
|---|---|---|---|
| HAL / peripherals | `esp-hal` | **1.1.2** (2026-08-05) | Espressif's own, vendor-backed. 1.0 shipped Oct 2025. Stable API surface = init + GPIO/UART/SPI/I2C; everything else (ADC, PCNT, RNG, RWDT…) is behind the `unstable` feature — works fine, API may still move. |
| Allocator | `esp-alloc` | tracks esp-hal | Required by `esp-radio`. Gives you `alloc` (`String`, `Vec`, `Box`) in `no_std`. |
| Scheduler | `esp-rtos` | 0.1.x | Required by `esp-radio`. Provides preemptive tasks + the Embassy executor + the `embassy-time` driver, under the `embassy` feature. |
| Async runtime | `embassy-executor`, `embassy-time` | current | Driven by `esp-rtos`. |
| Panic / backtrace | `esp-backtrace` | 0.16.x | Panic + exception handlers with symbolised backtrace. Supports ESP32. **The Arduino build has nothing equivalent** — this is a strict improvement. |
| Boot / partitions | `esp-bootloader-esp-idf` | current | Partition-table parsing at runtime; needed to locate the config and buffer partitions. |

Route A foundation instead: `esp-idf-sys` + `esp-idf-hal` (0.46.x) + `esp-idf-svc` (**0.52.1**,
2026-03-10). No `esp-alloc`/`esp-rtos`/Embassy needed — you get `std`, threads and FreeRTOS directly.

## Per-module mapping

### `sense/anemometer.*` — reed-switch pulse counting (GPIO27, internal pull-up)

Today: `attachInterrupt` + a `volatile` counter, reset each cycle.

- **Route B, recommended:** `esp-hal`'s **PCNT** peripheral (`unstable` feature). The ESP32 has
  dedicated pulse-counter hardware with a built-in glitch filter — strictly better than an ISR for a
  mechanical reed switch, which bounces. `esp-idf-hal` has an equivalent `pcnt` module with a
  `pcnt_rotary_encoder.rs` example if you go Route A.
- **Fallback:** `esp-hal` GPIO interrupt + an `AtomicU32`, a direct translation of today's code.
- Wiring is already documented in `docs/hardware/wind-sensor-wiring.md` and does not change.

### `sense/vane.*` — 8-octant wind vane on ADC (GPIO34)

Today: `analogRead` compared against a `kOctantAdcReadings` table (still placeholder values).

- **Route B:** `esp_hal::analog::adc` (`unstable`). GPIO34 is ADC1_CH6. Needs `Attenuation::_11dB`
  for the full 0–3.3 V swing produced by the external 10 kΩ pull-up.
- **Route A:** `esp_idf_hal::adc::oneshot` — `AdcDriver` + `AdcChannelConfig`, with built-in
  calibration curves (a mild advantage: ESP32 ADC linearity is poor, and IDF's calibration is real).
- The octant lookup itself is pure logic and moves to `gustik-core` unchanged.

### `sense/magnetometer.*` — QMC5883P over I2C (`0x2C`, SDA=21, SCL=22)

**Decision (Q4): use the [`qmc5883p`](https://docs.rs/qmc5883p/) crate**, 1.0.1 (Jan 2026),
MIT/Apache-2.0, `embedded-hal-async` I2C, `no_std`, builder-configurable ODR/range/oversampling, with
a built-in self-test.

- **Its register map was checked against yours before recommending it** and matches on every point
  that caused trouble before: address `0x2C`, CTRL1 `0x0A`, CTRL2 `0x0B`, data from `0x01`, and the
  `0x29 = 0x06` axis-sign init. The one omission is the undocumented `0x0D = 0x40` write from QST's
  reference sequence, which your own bench notes say the sensor works without. Full comparison table
  in [03 § 5](03-risks-and-gaps.md#5-third-party-magnetometer-driver--checked-low-risk).
- Caveat worth knowing: 76 lifetime downloads, one author. The upside of the decision is that bugs
  found here go upstream; the downside is you may be the one finding them. Keep
  `firmware/src/sense/magnetometer.cpp` and `scripts/src/gustik_scripts/qmc5883p.py` as your
  reference oracles — if readings look wrong, diff against those rather than against the datasheet.
- **Carry over what the crate cannot know:** the hard-iron offsets from
  `scripts/qmc5883p-calibration.json` and the Y-axis sign flip for the confirmed mount (`up=-z`,
  `forward=+x`). Apply the flip at the call site in `main.rs`, keeping it out of `gustik-core` —
  the same boundary the C++ code deliberately draws.
- This is also where `bug-030` (I2C hang) gets structurally fixed: `esp-hal`'s I2C operations return
  `Result` and have a bus timeout, and Rust's `#[must_use]` makes silently ignoring the return code
  a compiler warning rather than the default.
- Do **not** reach for `qmc5883l` or `hmc5983` crates — different chips, different register maps.
  That mistake already happened once here (`bug-029`).

### `correct/*` and the pure-logic half of `transmit/*` and `config/*`

`pulsesToWindSpeedMs`, `magnetometerHeadingDegrees`, `correctWindDirectionOctant`,
`RingBufferIndex`, `ConnectionMonitor`, `RssiAvailabilityLatch`, `shouldLedSignalDisconnect`,
`computeBufferCapacityForHours`, the `config.txt` parser and `selectNetworkIndex`.

**No crates needed.** These become `gustik-core`: `#![no_std]`, `#![cfg_attr(test, ...)]`, depends on
nothing but `heapless` and maybe `libm` (for `atan2` in the heading calculation — `core` has no
floating-point transcendentals in `no_std`; `libm` is the standard answer, and `esp-hal` pulls it in
anyway). The 43 Unity tests become ordinary `#[test]` functions and run under plain
`cargo test` on the host — no PlatformIO, no `test_build_src=yes`, no per-test-directory `main()`.
This is a clear ergonomic win over the current `pio test -e native` setup.

### `transmit/payload.*` — JSON body construction

Today: hand-built JSON string.

- **`serde` + `serde-json-core`** — `no_std`, zero-alloc serialisation into a `heapless::Vec`.
  `#[derive(Serialize)]` on a `Reading` struct with `#[serde(rename_all = "camelCase")]` replaces the
  manual string building and eliminates a whole class of escaping bug.
- Route A can use plain `serde_json` since `std` is available.

### `transmit/hw/wifi_client.*` — Wi-Fi association + HTTPS POST

Two separate concerns:

**Wi-Fi association / network selection.**
- Route B: `esp-radio` `WifiController` — `scan_n()`, `set_configuration()`, `connect()`, all async
  under Embassy, plus `embassy-net` for DHCP and DNS. The existing `selectNetworkIndex` priority
  logic ports unchanged into `gustik-core`, fed by scan results. The bounded per-network connect
  retry added for `bug-030` maps naturally onto `embassy_time::with_timeout`.
- Route A: `esp_idf_svc::wifi::{EspWifi, BlockingWifi}` — near-identical to `WiFi.h`.

**HTTPS POST.**
- Route B: **`reqwless`** over `embassy-net`, with **`embedded-tls`** and certificate verification
  disabled (Q3 decision — see
  [03 § 1](03-risks-and-gaps.md#1-tls--resolved-unverified-tls-13-accepted)). Both crates.io, no git
  dependencies. Verified that the live backend completes a handshake with exactly the cipher suite
  and key-exchange group `embedded-tls` implements (`TLS_AES_128_GCM_SHA256` + X25519), and serves an
  ECDSA rather than RSA certificate, which keeps the handshake cheap.
- Route A: `esp_idf_svc::http::client::EspHttpConnection` with `crt_bundle_attach`. Full CA
  verification, hardware-accelerated, no drama. Roughly 15 lines.

### `transmit/hw/clock.*` — NTP + ISO-8601 timestamps

- Route B: **`sntpc`** (async-first, `no_std`, works over an `embassy-net` UDP socket; a `sync`
  submodule exists too) to get the epoch, then hold the offset against `embassy_time::Instant`.
  Formatting the ISO-8601 string: `chrono` with `default-features = false`, or just
  `core::write!` into a `heapless::String` — the format is fixed and trivial.
- Route A: `esp_idf_svc::sntp::EspSntp`, then `std::time::SystemTime`.

### `transmit/hw/flash_buffer.*` — ≥4 h persistent reading buffer

Today: LittleFS, one small pipe-delimited file per ring slot (`/buf/<slot>.txt`), with slot
bookkeeping in the separately-tested `RingBufferIndex`.

- **Route B, recommended:** [`esp-storage`](https://crates.io/crates/esp-storage) (implements
  `embedded-storage` over raw ESP32 flash; ESP32 explicitly supported) +
  [`sequential-storage`](https://crates.io/crates/sequential-storage)'s **queue** API. That crate
  provides exactly a persistent, power-loss-safe, wear-levelled FIFO queue over a flash range —
  which is what `flash_buffer` + `RingBufferIndex` are re-implementing by hand. You would push
  serialised readings and pop them after a successful backfill, and `RingBufferIndex` could
  arguably be deleted rather than ported. Alternative if you want key-value instead of a queue:
  [`ekv`](https://crates.io/crates/ekv).
- **If you want a real filesystem instead:** `littlefs2` is `no_std` and works, but you must
  implement its `Storage` trait over `esp-storage` yourself, and there are reports of it being
  fiddly on ESP32. Only worth it if you specifically want file semantics — for a FIFO you don't.
- Route A: `std::fs` over an ESP-IDF LittleFS VFS mount — an almost literal port of today's code.

### `config/hw/config_loader.*` — read `config.txt` from flash

- Route B: no filesystem. Put the config bytes in a dedicated data partition, read it with
  `esp-storage`, feed the bytes to the **unchanged** `gustik-core` parser. Provisioning becomes
  `espflash write-bin <offset> config.txt` instead of `pio run -t uploadfs`. See
  [03](03-risks-and-gaps.md#3-config-provisioning-loses-uploadfs).
- Note this also structurally removes `bug-028` (the SPIFFS-vs-LittleFS image mismatch): there is no
  filesystem image to get wrong.
- Route A: `std::fs::read_to_string`, same as today.

### `main.cpp` — LEDs and serial diagnostics

- LEDs (GPIO2 disconnect, GPIO25 config-loaded, GPIO26 wifi-connected): `esp_hal::gpio::Output`,
  trivial.
- Serial diagnostics: **`esp-println` with the `log` feature**, over UART0 at 115200.
  Read with `espflash monitor` (or any terminal — it's plain text).
  - **Deliberately not `defmt`.** `defmt` is more efficient and is the fashionable choice, but its
    output is a binary framing that only `espflash --log-format defmt` can decode. Every field bug in
    this project so far was diagnosed by a human reading two plain lines per cycle off a serial
    monitor, sometimes on a boat. Keep that property. (`esp-println` does support `defmt-espflash`
    later if you change your mind, and there is a known wart where `esp-backtrace`'s panic output and
    `defmt` don't cooperate cleanly.)
- The 3 s sample loop becomes an Embassy task with `Ticker::every(Duration::from_secs(3))` — which
  fixes a latent wart in the current code, where `loop()` busy-polls `millis()` and the actual
  interval drifts with whatever the cycle did.

## Tooling

| Task | Today | Rust |
|---|---|---|
| Toolchain install | `uv tool install platformio` | `cargo install espup && espup install` (+ `cargo install espflash`) |
| Scaffold | — | `esp-generate` (`no_std`; supports ESP32) or `cargo generate esp-rs/esp-idf-template` (`std`) |
| Build | `pio run -e esp32dev` | `cargo build --release` (with `rust-toolchain.toml` pinning `esp`) |
| Flash | `pio run -t upload` | `cargo espflash flash --release` |
| Monitor | `pio device monitor` | `espflash monitor` (or `cargo espflash flash --monitor`) |
| Upload config | `pio run -t uploadfs` | `espflash write-bin <offset> config.txt` |
| Host tests | `pio test -e native` (Unity) | `cargo test -p gustik-core` (stable toolchain, no ESP anything) |
| CI | existing build job | add `esp-rs/xtensa-toolchain` action for the firmware build; `cargo test` needs no action at all |

`/dev/ttyUSB0` already works from this machine (`dialout` granted), so flashing and monitoring need
no new access.

## Sources

- [esp-hal](https://github.com/esp-rs/esp-hal) · [crates.io/esp-hal](https://crates.io/crates/esp-hal) · [esp-hal 1.0 announcement](https://developer.espressif.com/blog/2025/10/esp-hal-1/)
- [esp-radio (esp32 docs)](https://docs.espressif.com/projects/rust/esp-radio/0.16.0/esp32/esp_radio/index.html) · [crates.io/esp-radio](https://crates.io/crates/esp-radio)
- [esp-rtos docs](https://docs.espressif.com/projects/rust/esp-rtos/0.1.0/esp32/esp_rtos/index.html) · [Rust on ESP Book — Async](https://docs.espressif.com/projects/rust/book/application-development/async.html)
- [qmc5883p crate](https://docs.rs/qmc5883p/latest/qmc5883p/) ([repo](https://github.com/KapJ1coH/qmc5883p))
- [esp-storage](https://crates.io/crates/esp-storage) · [littlefs2](https://docs.rs/littlefs2)
- [reqwless](https://github.com/drogue-iot/reqwless) · [sntpc](https://github.com/vpetrigo/sntpc) · [embassy-net](https://docs.embassy.dev/embassy-net/)
- [esp-println](https://docs.rs/esp-println/latest/esp_println/) · [esp-backtrace](https://github.com/esp-rs/esp-backtrace)
- [esp-idf-hal examples (adc.rs, pcnt_rotary_encoder.rs)](https://github.com/esp-rs/esp-idf-hal/tree/master/examples) · [esp-idf-svc http_client.rs](https://github.com/esp-rs/esp-idf-svc/blob/master/examples/http_client.rs)
