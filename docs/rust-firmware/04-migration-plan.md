# 04 — Migration plan

Route B (`no_std` + esp-hal + esp-radio + Embassy) on the existing ESP32-WROOM, as a parallel tree.

> **Revised 2026-08-14** after Mlok's answers to Q1–Q4: chip stays ESP32-WROOM, `no_std` confirmed,
> TLS without certificate verification (no git deps), magnetometer via the `qmc5883p` crate. The
> `gustik-drivers` crate is gone and phase 3's estimate came down accordingly.

## Ground rules

1. **`firmware/` is not touched.** It is the deployed, field-verified artifact. The Rust work lives
   in a new top-level `firmware-rs/`. Both can exist indefinitely.
2. **Nothing merges to `main` that changes what gets flashed to the boat.** CI may build the Rust
   tree; it must not deploy it.
3. **Every phase is independently useful and independently abandonable.** If phase 3 (TLS) turns out
   to be a swamp, phases 1–2 still stand on their own as a completed piece of learning.
4. Calibration constants and pin assignments are ported **verbatim**, TODO comments included, so the
   Rust and C++ builds can be compared against each other on the same hardware.

## Workspace layout

```
firmware-rs/
├── Cargo.toml                 # workspace
├── rust-toolchain.toml        # channel = "esp"
├── partitions.csv             # custom single-app table: app / config / buffer
├── config.example.txt         # same format as firmware/data/config.example.txt
├── crates/
│   └── gustik-core/           # #![no_std], NO hal dependency — pure logic + tests
│       ├── src/
│       │   ├── wind_speed.rs        # <- correct/wind_speed.*
│       │   ├── wind_direction.rs    # <- correct/wind_direction.*
│       │   ├── config.rs            # <- config/station_config.*
│       │   ├── connection.rs        # <- transmit/connection_monitor.*, led_policy.h, rssi_latch.h
│       │   ├── buffer.rs            # <- transmit/ring_buffer_index.h, buffer_capacity.*
│       │   ├── ingest_response.rs   # <- transmit/ingest_response.* (the 2026-08-14 addition)
│       │   └── reading.rs           # <- transmit/reading.h + payload.* (serde)
│       └── tests/                   # <- the 57 Unity tests, as #[test]
└── src/main.rs                # the binary: esp-hal + esp-radio + Embassy + qmc5883p wiring
```

Why `gustik-core` has no HAL dependency: it makes phase 1 buildable and testable on a stock stable
toolchain with no ESP32 anything, **and** it is what makes a late pivot to Route A cheap — only
`src/main.rs` would be rewritten.

The earlier draft had a second `gustik-drivers` crate for a hand-written magnetometer driver. **Q4
chose the `qmc5883p` crate instead**, so that crate is gone — the driver is now a dependency, and the
calibration (hard-iron offsets, Y-axis sign flip for the confirmed mount) is applied at the call site
in `main.rs`, keeping it out of `gustik-core` exactly as the C++ code keeps it out of
`correct/wind_direction.cpp`.

Note `transmit/ingest_response.*` — added to the C++ firmware on 2026-08-14, after this study
started. It is pure logic with no `Arduino.h`, so it ports into `gustik-core` with everything else,
and it is what makes the Serial line report `stored=1/1` rather than a bare `sent=yes`. Port it; it
is the fix for the `bug-031` failure mode being invisible.

## Phases

### Phase 1 — pure logic, no hardware, no toolchain *(low risk, high value)*

Port `gustik-core` and its tests. Run with plain `cargo test` on this machine. No `espup`, no ESP32,
no Wi-Fi.

- Port all 57 tests (43 at the time of the original study, plus 14 added with
  `transmit/ingest_response.*` on 2026-08-14). They should pass with the same expectations and the
  same numbers.
- Use `serde` + `serde-json-core` for the reading payload, and assert the serialised JSON matches
  what `transmit/payload.cpp` produces byte-for-byte for a known reading. That's a real
  cross-implementation check, not a tautology.
- Deliverable: a crate that proves the maths is identical, plus your first real exposure to
  `no_std`, `heapless`, `libm`, and Rust's test tooling.

**Stop here and you have already got something.**

### Phase 2 — blinky, then sensors *(medium risk)*

`espup install`, `cargo install espflash`, scaffold with `esp-generate`, get the three diagnostic
LEDs blinking on the real device. Then, one at a time:

1. UART diagnostics via `esp-println` + `log`, and `esp-backtrace` wired up. Prove you can read
   output with `espflash monitor` before anything else.
2. Anemometer via PCNT (or a GPIO interrupt + `AtomicU32` first, if PCNT fights you).
3. Vane via ADC1_CH6 on GPIO34, `Attenuation::_11dB`.
4. Magnetometer via the `qmc5883p` crate, with the hard-iron offsets and Y-axis sign flip applied at
   the call site. Validate by rotating the board and checking the yaw tracks — the same check that
   verified the C++ version on 2026-08-12. If the readings look wrong, the first thing to try is
   adding the `0x0D = 0x40` write the crate omits (see
   [03 § 5](03-risks-and-gaps.md#5-third-party-magnetometer-driver--checked-low-risk)); that would
   also be the first upstream contribution.

Deliverable: a firmware that reads all three sensors and prints the same per-cycle diagnostic line
the C++ build prints, with no network at all. At this point you can run both firmwares on the bench
and diff the numbers.

### Phase 3 — network *(medium risk since Q3; was the scary one)*

1. Wi-Fi association via `esp-radio` + `embassy-net`, DHCP, DNS.
2. **A 40-line spike: one hardcoded HTTPS POST to the real backend, print the status code.**
   `reqwless` + `embedded-tls` with verification disabled — both from crates.io, no git deps.
   Already known to be protocol-compatible: the live backend was verified to complete a TLS 1.3
   handshake restricted to `TLS_AES_128_GCM_SHA256` + X25519, which is what `embedded-tls` offers.
   Still do the spike before the application logic — "protocol-compatible" and "works on the chip
   with 16 kB of TLS buffers alongside the Wi-Fi stack" are different claims.
3. Only then: NTP via `sntpc`, ISO-8601 formatting, config read from its flash partition,
   `selectNetworkIndex` wired to real scan results, the sample loop as an Embassy `Ticker` task, and
   `ingest_response` parsing so the diagnostics line reports `stored=n/m` like the C++ build does.

Deliverable: continuous live readings landing in `https://gustik.remesh.cz`, verifiable exactly the
way the C++ firmware was — `curl https://gustik.remesh.cz/readings/latest` and check the timestamp
tracks wall-clock.

### Phase 4 — resilience parity *(low risk, mostly application logic)*

The Epic 2 behaviours: flash buffer via `esp-storage` + `sequential-storage` queue, backfill on
reconnect, disconnect LED, RSSI latch. Then verify against the same acceptance criteria the C++
stories used — pull the Wi-Fi, confirm buffering, restore it, confirm backfill arrives with the
`backfilled` flag set on the backend.

Deliverable: behavioural parity. Only at this point is a "should the Rust build become the real
firmware?" conversation worth having, and the answer should still involve a full season of bench
soak time first.

## Testing strategy

| Layer | How | Compared to today |
|---|---|---|
| Pure logic | `cargo test -p gustik-core` on the host, stable toolchain | Replaces `pio test -e native` / Unity. No `test_build_src=yes`, no one-`main()`-per-test-directory, no `-DUNITY_INCLUDE_DOUBLE`. Strictly better. |
| Magnetometer | The `qmc5883p` crate's own tests + its built-in hardware self-test, exercised at boot | The crate ships a self-test routine; `sense/magnetometer.cpp` has none. Wire it into the boot diagnostics line next to `magnetometer init: ok`. |
| Payload | Golden-file assertion against the C++ output for a fixed reading | New; catches serialisation drift between the two implementations. |
| Hardware | Manual, on the bench device via `espflash monitor`, same checks the C++ stories used | Unchanged in kind. |
| Live | `curl https://gustik.remesh.cz/readings/latest` | Unchanged. |

## CI

Add to `.github/workflows/ci.yml`, guarded so it never gates a deploy:

- `cargo test -p gustik-core` — needs nothing but `dtolnay/rust-toolchain@stable`. Cheap, fast, and
  covers the majority of the ported code.
- `cargo build --release` for the firmware binary — needs `esp-rs/xtensa-toolchain`. Slower;
  reasonable to run only on changes under `firmware-rs/`.
- `cargo clippy` and `cargo fmt --check`.

The existing PlatformIO firmware job and the backend deploy path stay exactly as they are.

## Rough effort

| Phase | Estimate | Dominated by |
|---|---|---|
| 1 | an evening or two | mechanical translation; ~570 LOC + 530 lines of tests |
| 2 | a weekend | first-time `espup`/`esp-generate` setup, PCNT and ADC APIs being `unstable`-gated |
| 3 | a weekend | first-time Embassy/`esp-radio` wiring. **Revised down** — the Q3 decision removed the git-dep TLS integration that made this open-ended |
| 4 | a weekend | `sequential-storage` learning curve; the logic itself is already written and tested in phase 1 |

Since Q1–Q4 were answered, the plan has no open-ended item left in it. The largest remaining
unknown is first-time `esp-radio` setup in phase 3, which is a known-quantity kind of hard rather
than a "does this even work" kind of hard.
