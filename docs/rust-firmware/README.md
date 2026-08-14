# Rust firmware rewrite — feasibility study

**Date:** 2026-08-14 (revised same day after Mlok's answers) · **Status:** study only, nothing
implemented · **Requested by:** Mlok, as a personal learning exercise

## The question

Can the Gustik station firmware (currently C++/Arduino on an ESP32-WROOM, ~1500 LOC, PlatformIO)
be rewritten in Rust? What crates exist for the things this firmware actually does — Wi-Fi, HTTPS,
the QMC5883P magnetometer, flash buffering, NTP, serial diagnostics — and how mature are they?

## Short answer

**Yes.** Every capability this firmware needs has a real crate today, most of them first-party
Espressif code, and after Mlok's decisions **all of them come from crates.io under normal semver** —
no git dependencies, no unpublished crates.

The honest framing is that this is a rewrite of working, field-verified firmware at the very moment
of the target regatta deployment (`brief.md` targets mid-August 2026; today is 2026-08-14). The plan
therefore treats the Rust port as a *parallel* artifact in `firmware-rs/`, with the C++ firmware
staying the deployed one. See [04-migration-plan.md](04-migration-plan.md).

## Decisions taken

| # | Question | Answer |
|---|---|---|
| Q1 | Chip | **Stay on the ESP32-WROOM** (Xtensa). RISC-V considered, none at hand. |
| Q2 | `no_std` or `std` | **`no_std`** — Route B, bare metal on `esp-hal`. |
| Q3 | TLS | **`embedded-tls` without certificate verification.** Encryption defeats passive token sniffing; a hostile AP is not in this deployment's threat model. |
| Q4 | Magnetometer | **Use the `qmc5883p` crate**, not a hand-written driver — bugs go upstream. |

Q3 in particular removed what had been the study's one real gap. Both remaining network crates
(`reqwless`, `embedded-tls`) are published, and mbedTLS is out of the flash budget entirely.

Q5–Q8 (second bench board, `defmt` vs plain text, repo placement, whether this ever ships) are still
open in [05-open-questions.md](05-open-questions.md) — none of them block starting.

## Documents

| File | Contents |
|---|---|
| [01-feasibility.md](01-feasibility.md) | What the firmware has to do, the two routes, Xtensa toolchain reality, the chosen route |
| [02-crate-inventory.md](02-crate-inventory.md) | Capability-by-capability crate mapping against the *actual* firmware modules, with versions and maturity notes |
| [03-risks-and-gaps.md](03-risks-and-gaps.md) | TLS decision + verification evidence, RAM/flash budget, provisioning ergonomics, and a scorecard of which past Gustik bugs Rust would and would not have prevented |
| [04-migration-plan.md](04-migration-plan.md) | Workspace layout, four phases, test strategy, CI |
| [05-open-questions.md](05-open-questions.md) | Q1–Q4 answered with consequences; Q5–Q8 still open |

## The plan in one paragraph

A new `firmware-rs/` Cargo workspace beside `firmware/`, built on `esp-hal` 1.1.x + `esp-radio` +
`esp-rtos` + Embassy. All ~570 LOC of pure logic goes into a `gustik-core` crate with **no HAL
dependency**, so it builds and runs its 57 ported tests on the host under stock stable Rust —
**phase 1 needs no `espup`, no ESP32, and cannot break anything.** Phase 2 brings up the sensors,
phase 3 the network, phase 4 the buffering and backfill behaviour. The C++ firmware is not touched
at any point.

## Verified rather than assumed

Two claims in here were checked against reality rather than inferred, because both are load-bearing:

- **TLS:** `curl --tls13-ciphers TLS_AES_128_GCM_SHA256 --curves X25519 https://gustik.remesh.cz/health`
  returns 200, i.e. the live backend completes a handshake restricted to exactly what `embedded-tls`
  implements. It also serves an ECDSA (not RSA) certificate, which keeps the handshake cheap.
- **Magnetometer:** the `qmc5883p` crate's register map matches the map already proven on your board
  (`0x2C`, CTRL1 `0x0A`, CTRL2 `0x0B`, data at `0x01`, `0x29 = 0x06`). It omits the undocumented
  `0x0D = 0x40` write your two implementations do — which your own bench notes say the sensor works
  without.
