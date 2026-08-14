# 03 — Risks and gaps

> **Revised 2026-08-14** after Mlok answered Q1–Q4 (see [05](05-open-questions.md)). Two items that
> were the study's biggest risks — TLS and the third-party magnetometer driver — are now resolved
> decisions with verification behind them, and are kept here as records rather than warnings.
> **The remaining top risk is the Xtensa toolchain (§4), followed by the RAM/flash budget (§2).**

Ordered as originally written, most-likely-to-cost-a-weekend first; §1 and §5 have since been
downgraded.

## 1. TLS — resolved: unverified TLS 1.3, accepted

**Decision (Mlok, 2026-08-14): use `embedded-tls` without certificate verification.** The rationale,
in his words: encryption still prevents the bearer token being sniffed off the air, and a malicious
AP is not part of the threat model for a committee boat on a small lake. Recorded as a deliberate,
informed risk acceptance.

This is the single biggest simplification to the plan. It was the study's highest-risk item; it is
now an ordinary one:

- **No git dependencies.** `reqwless` and `embedded-tls` are both on crates.io with normal semver.
  The alternative (`esp-mbedtls`, which *does* verify certificates and has ESP32 hardware crypto
  acceleration) is unpublished and would have forced pinned git revisions on two crates.
- **No mbedTLS in the flash budget** — meaningful on a chip whose C++ build already sits at 92.3 %.
- Phase 3 stops being the phase that decides whether the whole route is viable.

### Verified against the live backend, not assumed

`embedded-tls` is TLS 1.3-only and implements a narrow set of primitives, so "Caddy serves TLS 1.3"
is not by itself enough. Checked directly against `https://gustik.remesh.cz` on 2026-08-14:

```
curl --tls13-ciphers TLS_AES_128_GCM_SHA256 --curves X25519 https://gustik.remesh.cz/health
→ http=200
```

That forces the handshake into exactly the cipher suite and key-exchange group `embedded-tls`
supports, and the server completes it. Two further findings from the same check:

- The server's default negotiation with a modern client picks `X25519MLKEM768` (post-quantum hybrid)
  and ChaCha20-Poly1305. `embedded-tls` offers neither, but Caddy falls back cleanly to classical
  X25519 + AES-128-GCM, as the forced handshake above proves.
- The certificate is **ECDSA** (`id-ecPublicKey`), not RSA. This matters even without verification,
  because the certificate still has to be parsed: `reqwless`'s own notes call out `alloc` as a
  requirement for HTTPS *with RSA certificates*. An EC certificate keeps the handshake cheap.
  (`esp-radio` requires an allocator anyway, so `alloc` is available regardless — this is about
  handshake cost and RAM, not availability.)

### What is actually given up, stated plainly

An attacker who can put the station on an AP they control can terminate TLS, read the ingest token,
and post fabricated readings to the dashboard. They cannot read anything private — the dashboard is
public and unauthenticated by design — and they cannot reach the backend host beyond the ingest
endpoint. That is the whole exposure, and it is the exposure Mlok has accepted.

Two things worth keeping in mind rather than acting on now:

- If the ingest token ever guards something that matters more than a regatta graph, revisit this.
  The upgrade path is `esp-mbedtls` (git dep) or Route A's `crt_bundle_attach`, both of which slot in
  behind the same `reqwless` call site.
- Rotating the ingest token is cheap (`INGEST_TOKEN` is a repo secret written to the server's `.env`
  on every deploy, plus a `config.txt` edit and a `write-bin` on the device), so if it ever does leak
  the remedy is routine.

### Residual, minor

`embedded-tls` needs its own record buffers — TLS 1.3 records go up to 16 kB, so budget roughly
16 kB of read buffer plus a smaller write buffer, on a 520 kB chip that is also running Wi-Fi and
smoltcp. Fine for one small POST every 3 s to one host, which is Gustik's exact shape. Don't hold the
session open across the whole loop if RAM gets tight. It also needs an RNG seed — `esp-hal`'s `Rng`
provides one (a true RNG once the radio is up).

## 2. RAM and flash budget on an ESP32-WROOM

- **Flash:** the current C++ build sits at **92.3 %** of the default 1.31 MB app partition (~101 kB
  headroom). Rust + esp-radio + `embedded-tls` should land in a similar range — and notably smaller
  than it would have with mbedTLS, which the Q3 decision removed. There is no OTA mechanism in this
  project, so a custom single-app partition table recovers ~600 kB immediately — already flagged as
  an option in `firmware/platformio.ini`. Plan for a custom partition table from day one in the Rust
  tree; you need one anyway for the config and buffer partitions.
- **RAM:** 520 kB SRAM total, and Wi-Fi + smoltcp + a TLS session is the classic ESP32 squeeze.
  Budget ~16 kB for `embedded-tls`'s read buffer plus a smaller write buffer on top of the Wi-Fi
  stack. Gustik's shape helps a lot here: **one small POST every 3 s to one host, never concurrent**.
  Keep it that way — do not hold a TLS session open across the whole loop if memory gets tight, and
  don't add BLE.
- **esp-radio gotchas, from its own docs:** must be built at `opt-level` 2 or 3 or connectivity
  becomes unreliable; needs `esp-alloc` and a preemptive scheduler (`esp-rtos`); if any radio work
  lands on core 1, budget ≥16 kB of stack for it. These are the kind of thing that produce a day of
  confused debugging if you don't know them in advance.

## 3. Config provisioning loses `uploadfs`

Today: edit `firmware/data/config.txt`, run `pio run -t uploadfs`, done. That workflow is used by a
human on a boat and it matters.

Route B has no filesystem, so the equivalent is: keep `config.txt` as a plain file, define a data
partition for it in the partition table, and flash it with `espflash write-bin <offset> config.txt`.
The `gustik-core` parser is unchanged. Functionally equivalent, one more thing to remember, and
easy to wrap in a `just`/`cargo xtask` recipe so the day-to-day command stays one line.

Silver lining: this structurally removes `bug-028` (`uploadfs` silently building a SPIFFS image while
the firmware mounted LittleFS). No filesystem image, nothing to mismatch.

Route A keeps `std::fs` and a VFS mount, i.e. essentially today's workflow.

## 4. Xtensa toolchain friction

Covered in [01](01-feasibility.md#the-toolchain-reality-xtensa). Summary of the day-to-day cost:
`espup install` is a large download; `rust-toolchain.toml` must pin `esp`; some editor/CI setups need
explicit pointing at it; and you are on a Tier-3 target, so when something breaks you are reading
GitHub issues rather than Stack Overflow. All manageable, all real.

The ESP32 also has **no USB-Serial-JTAG** (unlike the C3/S3), so `probe-rs`-style debugging needs an
external JTAG adapter. In practice you'll debug with `esp-println` + `esp-backtrace` — which is still
better than what the Arduino build offers.

## 5. Third-party magnetometer driver — checked, low risk

**Decision (Mlok, 2026-08-14): use the `qmc5883p` crate rather than writing a driver**, on the
reasoning that helping fix bugs upstream beats maintaining a private implementation.

The obvious worry was that this crate has 76 lifetime downloads and sits between you and the one
sensor that has already caused two incidents here (`bug-029` wrong chip, `bug-030` I2C hang). So I
diffed its register map against the two implementations already proven on your physical board:

| | crate `qmc5883p` 1.0.1 | `sense/magnetometer.cpp` + `scripts/…/qmc5883p.py` |
|---|---|---|
| I2C address | `0x2C` | `0x2C` ✅ |
| CTRL1 (mode + ODR) | `0x0A` | `0x0A` ✅ |
| CTRL2 (range + self-test) | `0x0B` | `0x0B` ✅ |
| Data start (X LSB) | `0x01` | `0x01` ✅ |
| Axis-sign init | writes `0x29 = 0x06` | writes `0x29 = 0x06` ✅ |
| QST init reg `0x0D` | **not written** | writes `0x0D = 0x40` ⚠️ |

So the crate targets the right chip with the right map — the `bug-029` class of failure is ruled out.
The single difference is the undocumented `0x0D = 0x40` write from QST's reference init sequence.
Your own bench notes say the sensor "also works without them in basic testing", so this is very
likely immaterial; if it turns out to matter on your hardware, it is a one-line upstream PR, which is
precisely the dynamic you were after.

Two things to carry over that the crate cannot know about: the **hard-iron offsets** from
`scripts/qmc5883p-calibration.json` and the **Y-axis sign flip** for the confirmed mount orientation
(`up=-z`, `forward=+x`). In the C++ firmware the sign flip lives in the hardware layer, deliberately
kept out of the pure `correct/wind_direction.cpp`. Keep that boundary: apply the flip at the call
site in `main.rs`, not inside `gustik-core`.

The crate is `embedded-hal-async`-only, which is a non-issue under Embassy but would be awkward if
you ever fell back to Route A.

## 6. Calibration constants are still placeholders

Independent of language: `metersPerSecondPerHz = 1.2` and `vane.cpp`'s `kOctantAdcReadings` are
un-measured placeholders (see `TODO.md`). Port them as-is with the same TODO markers. A rewrite is
*not* the moment to also change calibration — you'd lose the ability to compare Rust and C++ output
side by side on the same hardware, which is the cheapest possible correctness check for this port.

## 7. Schedule risk (the elephant)

`brief.md` targets a real regatta deployment by mid-August 2026. Today is 2026-08-14. The C++
firmware is live-verified end to end (magnetometer reading plausibly, Wi-Fi up, backend receiving
continuous data as of 2026-08-12).

**Do not put the Rust firmware anywhere near the boat this season.** The plan in
[04](04-migration-plan.md) is explicitly parallel: `firmware/` stays the deployed artifact and is not
modified; `firmware-rs/` is a separate tree that can be flashed onto the bench device (or a second
ESP32) whenever you feel like it. Nothing in the Rust work should be allowed to become a dependency
of the regatta.

## What Rust would — and would not — have prevented

Honest scorecard against the bugs this firmware actually had. It's a mixed result, which is worth
knowing before you assume the rewrite buys safety.

| Bug | Would Rust have prevented it? |
|---|---|
| `bug-030` — I2C hang: `Wire` calls with no timeout and unchecked return codes froze the whole loop silently | **Yes, largely.** `esp-hal` I2C returns `Result` and has a bus timeout; ignoring a `#[must_use]` `Result` is a compiler warning. The "unchecked return code" half is designed out. |
| `bug-028` — `uploadfs` built a SPIFFS image while firmware mounted LittleFS | **Yes, incidentally** — Route B has no filesystem at all. Not a Rust property, a design-change property. |
| `bug-029` — driver written for QMC5883L instead of the real QMC5883P | **No.** Wrong register map is wrong in any language. |
| `bug-031` / `bug-039` — `clientId` collided across reboots; firmware reported `sent=yes` while the backend silently discarded the row | **No.** A distributed-systems / API-contract bug. Rust's type system doesn't know that HTTP 201 was a lie. (Now fixed on both ends.) |
| Calibration placeholders | **No.** Physical measurement. |

The genuine wins are: `Result`-typed hardware APIs, a real panic handler with backtraces,
`cargo test` for the pure logic instead of PlatformIO's Unity harness, and `serde` instead of
hand-built JSON. Those are worth having. They are not a reason on their own to rewrite working
firmware — the learning goal is, and that's fine.
