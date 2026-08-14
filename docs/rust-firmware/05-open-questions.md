# 05 — Questions and answers

Original study written 2026-08-14 with assumed answers. **Mlok answered Q1–Q4 the same day**; the
rest of the documents have been re-spun to match. Q5–Q8 are still open but none of them block
starting.

## Answered

### Q1 — Which chip? → **Stay on the ESP32-WROOM (Xtensa)**

> *"The chip stays for now, I will consider the RISC-V alternative, but don't have one at hand."*

Confirmed as assumed. `espup` and the Espressif compiler fork are therefore part of the plan; see
[01 § toolchain](01-feasibility.md#the-toolchain-reality-xtensa). Nothing in the recommended workspace
layout is chip-specific except `src/main.rs` and the partition table — `gustik-core` would move to a
RISC-V board untouched, and the HAL API is the same `esp-hal` either way, so revisiting this later is
cheap. Filed as "revisit if the Xtensa toolchain becomes the thing you're fighting instead of Rust."

### Q2 — `no_std` or `std`? → **`no_std`** (Route B)

> *"I think no_std will be better."*

Confirmed as assumed. Route A (`std` on ESP-IDF) stays documented in
[01](01-feasibility.md#route-a--std-on-esp-idf) purely as a fallback, and the `gustik-core` split is
kept so that fallback stays cheap — but it is no longer the expected outcome.

### Q3 — TLS: git deps for certificate verification, or not? → **Skip verification, use `embedded-tls`**

> *"Security is not really important here. I am happy with unchecked certificate. Encryption is still
> good, because it prevents the API from being trivially exploitable (no sniff of the bearer token).
> I don't assume malicious actor with an AP here."*

**Accepted, and this is the single biggest simplification to the whole plan.** It removes the git
dependencies on `esp-mbedtls` and `drogue-iot/reqwless`, removes mbedTLS from the flash budget, and
turns the highest-risk phase into an ordinary one. Both `reqwless` and `embedded-tls` are on
crates.io with normal semver.

The reasoning holds up for this deployment: the station sits on a committee boat on a small lake,
the credential it carries is a write-only ingest token for a public-read dashboard, and the worst
realistic outcome of a stolen token is junk readings in a regatta graph. Passive sniffing is the
threat that actually exists, and encryption without verification stops exactly that.

I verified the mechanics against the live backend rather than assuming — see
[03 § TLS](03-risks-and-gaps.md#1-tls--resolved-unverified-tls-13-accepted). Short version: the real
server completes a TLS 1.3 handshake using precisely the cipher suite and key-exchange group
`embedded-tls` implements, and serves an ECDSA certificate rather than RSA, which keeps the handshake
cheap. So the approach is confirmed workable, not just permitted.

Recorded as a deliberate, informed risk acceptance in [03](03-risks-and-gaps.md), not as an oversight.

### Q4 — Own magnetometer driver, or the crate? → **The crate** (`qmc5883p`)

> *"Probably the crate, better to help resolve bugs than to write my own."*

Accepted, and it survived a check I ran before committing to it: **the crate's register map matches
the map that is already proven on your physical board.** Address `0x2C`, CTRL1 `0x0A`, CTRL2 `0x0B`,
data starting at `0x01`, and the `0x29 = 0x06` axis-sign write — all identical to
`firmware/src/sense/magnetometer.cpp` and to the bench-verified `scripts/` Python driver.

One difference: the crate does **not** write `0x0D = 0x40`, which both of your implementations do as
part of QST's reference init sequence. Your own bench notes in `scripts/src/gustik_scripts/qmc5883p.py`
say the sensor "also works without them in basic testing", so this is very likely a non-issue — and
if it isn't, it's a one-line upstream PR, which is exactly the contribute-back dynamic you were
after. Details in [02 § magnetometer](02-crate-inventory.md#sensemagnetometer--qmc5883p-over-i2c-0x2c-sda21-scl22).

Consequence for the plan: the separate `gustik-drivers` crate is dropped from the workspace layout.

## Still open (none are blocking)

### Q5 — A second ESP32 for the bench?

Doing phases 2–4 on the board that carries the live station means every experiment takes the station
offline. A second bare ESP32-WROOM (no sensors needed until phase 2 step 2) makes this much less
stressful, and is cheap. Not blocking: phase 1 needs no hardware at all, and phase 2 can start
whenever a board is free.

### Q6 — Diagnostics format: plain text or `defmt`?

**Assumed:** `esp-println` + `log`, plain human-readable UART, matching today's two-lines-per-cycle
format — because every field diagnosis on this project has been a human reading serial output,
sometimes on a boat. `defmt` is more efficient and is what most embedded-Rust material pushes you
toward, but decodes only through `espflash --log-format defmt`. Easy to switch later either way.

### Q7 — Where should this live?

**Assumed:** a new top-level `firmware-rs/`, sibling to `firmware/`. Keeps CI and the OpenWolf
bookkeeping in one place and makes A/B comparison against the C++ build easy. A separate repository
would keep the Gustik repo focused on the thing that actually flies. Your call — trivial to move
later.

### Q8 — Does this ever become the real firmware?

**Assumed: no, not this season, and possibly never.** `firmware/` stays the deployed artifact
throughout. If you *do* eventually intend the Rust build to replace it, that adds a soak-test phase
after phase 4 and changes how much phase 4 matters; if it's purely an exercise, phases 1–2 are where
essentially all the learning value is and phase 4 is optional.
