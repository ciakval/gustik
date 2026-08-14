# 01 — Feasibility and route selection

> **Revised 2026-08-14.** Mlok answered Q1–Q4 (see [05](05-open-questions.md)): stay on the Xtensa
> ESP32-WROOM, go `no_std`, accept TLS without certificate verification, use the `qmc5883p` crate.
> Route B below is therefore no longer a recommendation — it is the chosen route. Route A is kept
> as a documented fallback.

## What the firmware has to do

Taken from the real code, not from the brief. Sizes are lines of C++ (`.cpp` + `.h`), 2026-08-14:

| Module | LOC | What it does | Hardware-coupled? |
|---|---|---|---|
| `sense/` | 214 | Anemometer reed-switch pulse counting (ISR on GPIO27), wind-vane ADC read (GPIO34, 8 octants), QMC5883P magnetometer over I2C (`0x2C`, SDA=21/SCL=22) | yes |
| `correct/` | 79 | `pulsesToWindSpeedMs`, `magnetometerHeadingDegrees`, `correctWindDirectionOctant` — pure math | no |
| `transmit/` | 369 | JSON payload build, connection monitor, ring-buffer index, LED policy, RSSI latch, buffer capacity — pure logic | no |
| `transmit/hw/` | 295 | Wi-Fi connect + network selection, HTTPS POST of readings, NTP clock, LittleFS-backed ring buffer | yes |
| `config/` | 119 | `config.txt` key=value parser, network priority selector — pure logic | no |
| `config/hw/` | 29 | LittleFS read of `/config.txt` | yes |
| `main.cpp` | 189 | Wiring, 3 s sample loop, 3 diagnostic LEDs, Serial diagnostics | yes |
| `test/` | 530 | 43 Unity tests over the pure-logic modules, run on host via `pio test -e native` | n/a |

So: **~570 LOC of pure logic that is already hardware-free and tested**, and ~730 LOC of hardware
glue. That split is the single biggest reason this rewrite is tractable — it maps almost perfectly
onto a Cargo workspace with a `no_std`-but-host-testable core crate.

External requirements the firmware depends on:

- **HTTPS**, not HTTP. `firmware/data/config.txt` points at `https://` (the live backend is
  `https://gustik.remesh.cz` behind Caddy). `config.example.txt` still shows a plain-`http://`
  placeholder, but the real deployment is TLS. This was the largest risk in Route B until Q3 settled
  it — see [03 § 1](03-risks-and-gaps.md#1-tls--resolved-unverified-tls-13-accepted).
- **NTP**, because `capturedAt` is a real wall-clock ISO-8601 timestamp and the backend's history
  ordering depends on it.
- **Persistent flash buffer** covering ≥4 h of readings (NFR-4), surviving power loss.
- **Human-readable serial diagnostics.** Two lines per 3 s cycle. These were added during hardware
  bring-up and are how every field bug so far has been diagnosed. Non-negotiable.

## The toolchain reality: Xtensa

The ESP32-WROOM is **Xtensa LX6**, not RISC-V. As of August 2026:

- Upstream `rustc` lists `xtensa-esp32-none-elf` and `xtensa-esp32-espidf` as **Tier 3** targets in
  [the platform-support table](https://doc.rust-lang.org/nightly/rustc/platform-support.html), but
  the target spec existing upstream is not the same as being buildable upstream — the Xtensa codegen
  backend is not enabled in the LLVM that ships with stock `rustc`.
- Therefore you still install Espressif's fork with
  [`espup`](https://github.com/esp-rs/espup) (`espup install`), which lays down an `esp` toolchain
  (Rust fork + Espressif's LLVM fork + GCC), and pin it with `rust-toolchain.toml`. Espressif is
  actively upstreaming the LLVM changes, but it isn't done.

Practical consequences:

- `rustup target add` does not work for this chip; `espup` is a hard prerequisite.
- `cargo build` needs `+esp`; `rust-analyzer` and `clippy` work but must be pointed at that toolchain.
- **Host-side `cargo test` for the pure-logic crate uses ordinary stable Rust** and is completely
  unaffected. This is worth exploiting hard (see [04](04-migration-plan.md)).
- CI is fine: `esp-rs/xtensa-toolchain` is a maintained GitHub Action.

> **Worth knowing:** every one of these annoyances disappears on a RISC-V ESP32 (C3/C6/S3-is-still-Xtensa
> — C3 and C6 are RISC-V). Those build with plain `rustup target add riscv32imac-unknown-none-elf` on
> stable, no fork. The Gustik sensor interface is trivially portable (2 GPIO + I2C + 1 ADC), and a
> C6 devkit is a few hundred CZK. If the point is learning modern embedded Rust with the least
> incidental friction, doing it on a C6 is a legitimate option — see
> [05-open-questions.md](05-open-questions.md) Q1.

## Route A — `std` on ESP-IDF

`esp-idf-hal` + `esp-idf-svc` + `esp-idf-sys`. Rust with `std` (threads, `String`, `Vec`, sockets),
linked against a real ESP-IDF build. Closest possible analogue to the current Arduino firmware.

**Fit against Gustik's needs:** excellent, essentially 1:1.

- Wi-Fi: `EspWifi` / `BlockingWifi`, same underlying driver as Arduino's `WiFi.h`.
- HTTPS: `esp_idf_svc::http::client::EspHttpConnection` with
  `crt_bundle_attach: Some(esp_idf_svc::sys::esp_crt_bundle_attach)` — mbedTLS with the bundled Mozilla
  CA store and ESP32 hardware crypto acceleration. **Full certificate verification, out of the box.**
- NTP: `EspSntp`, one call.
- Config + buffer: ESP-IDF VFS with LittleFS or SPIFFS, or NVS. `std::fs` works.
- Diagnostics: `log` crate → `EspLogger` → UART0, human-readable.
- Sampling loop: `std::thread` + `std::time`, or just a blocking loop like today.

**Cost:** `esp-idf-sys` builds a full ESP-IDF via `embuild` — a multi-GB checkout and a long first
build; needs Python and the ESP-IDF prerequisites in whatever container you build in. Release cadence
is slower than the bare-metal side (`esp-idf-svc` 0.52.1, March 2026; before that 0.51.0 in Jan 2025).
Binary is larger — relevant given the current C++ build already sits at 91.9 % of the default 1.31 MB
app partition, though a no-OTA single-app partition table fixes that (already noted in
`firmware/platformio.ini`).

**Learning value: low-to-moderate.** You'd be writing Rust, but the concepts — a FreeRTOS task, a
blocking `esp_http_client`, a VFS mount — are the same ESP-IDF concepts as today. Almost nothing
here is the thing people mean by "embedded Rust".

## Route B — `no_std` bare metal on esp-hal

`esp-hal` 1.1.2 (Aug 2026, 1.0 released Oct 2025 — first vendor-backed Rust SDK, Espressif's own
team) + `esp-radio` (the renamed `esp-wifi`, at 1.0.0-beta.0 as of June 2026) + `esp-rtos` +
`embassy-executor` / `embassy-net` / `embassy-time`.

**Fit against Gustik's needs:** good.

- Wi-Fi: `esp-radio`, confirmed to support the original ESP32; exposes smoltcp traits, drives
  `embassy-net` (DHCP + DNS included).
- HTTPS: `reqwless` + `embedded-tls`, both crates.io, **with certificate verification off** — Mlok's
  explicit call (Q3), on the reasoning that encryption alone defeats passive token sniffing and a
  hostile AP is not in this deployment's threat model. Verified that the live backend completes a
  TLS 1.3 handshake restricted to what `embedded-tls` offers. See
  [03 § 1](03-risks-and-gaps.md#1-tls--resolved-unverified-tls-13-accepted). *This was the study's
  one real gap; the decision closed it.*
- NTP: `sntpc` over an `embassy-net` UDP socket.
- Flash buffer: `esp-storage` (raw flash via `embedded-storage` traits) +
  [`sequential-storage`](https://crates.io/crates/sequential-storage)'s queue API — which is a
  wear-levelled FIFO queue in a flash region, i.e. *literally what `transmit/hw/flash_buffer.cpp` +
  `ring_buffer_index.h` hand-rolls today*. This is an upgrade, not a workaround.
- Config: no filesystem. Read a raw data partition with `esp-storage`, flashed separately with
  `espflash write-bin`. Ergonomic regression vs `pio run -t uploadfs`; see
  [03](03-risks-and-gaps.md#3-config-provisioning-loses-uploadfs).
- Diagnostics: `esp-println` (`log` feature) over UART0 — plain readable text, same as today.
  `esp-backtrace` gives you a proper panic handler and backtrace, which the Arduino build does not.

**Cost:** more moving parts, more novel concepts, more chances to hit a rough edge. `esp-radio`
carries real gotchas (must build at `opt-level` 2 or 3 or Wi-Fi misbehaves; needs `esp-alloc` and a
scheduler; ≥16 kB stack if run on core 1).

**Learning value: high.** Embassy async tasks, `no_std`, the type-state HAL, `embedded-hal` traits,
allocator-free data structures with `heapless`, `Result`-everywhere hardware APIs. This is the
material.

## Decision

**Route B** — confirmed by Mlok, on the reasoning the study proposed: the stated goal is explicitly
"an exercise for myself", and Route A would teach comparatively little that is Rust-specific.
Concretely:

1. New `firmware-rs/` Cargo workspace, **alongside** `firmware/`. The C++ firmware remains the
   deployed artifact and is not touched.
2. `gustik-core` crate: `#![no_std]`, no HAL dependency, holds the ~570 LOC of pure logic and the
   ported tests. Builds and tests on the *host* with stable Rust. **This phase needs no `espup`,
   no ESP32, and cannot break anything.**
3. `gustik-firmware` binary crate: `no_std` + esp-hal + esp-radio + Embassy, depends on
   `gustik-core`.
4. Route A stays documented as the fallback. Its original trigger (TLS turning into a swamp) is
   mostly gone now that verification is off the table, but the core-crate split costs nothing and
   keeps the pivot cheap — only the binary crate would be rewritten against `esp-idf-svc`.

Effort estimate: roughly 1200–1800 LOC of Rust. Phase 1 (core + tests) is a comfortable evening or
two. Phases 2–4 are where the actual learning is, and the honest estimate is "several weekends",
now dominated by first-time Embassy/`esp-radio` setup rather than by TLS or the application logic.

## Sources

- [esp-hal on GitHub](https://github.com/esp-rs/esp-hal) · [esp-hal 1.0 release announcement](https://developer.espressif.com/blog/2025/10/esp-hal-1/) · [esp-hal on crates.io](https://crates.io/crates/esp-hal)
- [esp-radio docs (esp32)](https://docs.espressif.com/projects/rust/esp-radio/0.16.0/esp32/esp_radio/index.html) · [esp-radio on crates.io](https://crates.io/crates/esp-radio)
- [espup](https://github.com/esp-rs/espup) · [rustc platform support](https://doc.rust-lang.org/nightly/rustc/platform-support.html)
- [esp-idf-svc](https://crates.io/crates/esp-idf-svc) · [esp-idf-svc HTTPS client example](https://github.com/esp-rs/esp-idf-svc/blob/master/examples/http_client.rs) · [std-training HTTPS chapter](https://docs.esp-rs.org/std-training/03_3_3_https_client.html)
- [reqwless](https://github.com/drogue-iot/reqwless) · [esp-storage](https://crates.io/crates/esp-storage) · [sntpc](https://crates.io/crates/sntpc)
