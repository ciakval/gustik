# Magnetometer (QMC5883P / "GY-271") → ESP32 wiring

The yaw-correction magnetometer is a small breakout board, usually
silkscreened **HMC5883L** and sold as **GY-271** (or GY-273), that in fact
carries a **QMC5883P** die answering at I2C address **`0x2C`**. This was
confirmed 2026-08-11 on the real board with an MCP2221 USB-I2C bridge —
see `scripts/README.md` and `scripts/src/gustik_scripts/qmc5883p.py`, and
buglog `bug-029` for the earlier wrong-chip assumption. It is **not**
register- or address-compatible with a genuine HMC5883L (`0x1E`) or a
QMC5883L (`0x0D`); do not trust the silkscreen, trust the address that
answers.

Unlike the wind sensor head (passive reed switches — see
[wind-sensor-wiring.md](wind-sensor-wiring.md)), this is an active I2C
device: it needs power, and the bus needs pull-up resistors. Those
resistors are the one part of this wiring that is easy to get wrong, so
they get their own section below.

## Module pinout

GY-271/GY-273 boards carry 5 pins in this order (some clones omit `DRDY`
and have 4):

| Module pin | Meaning                | Connect to                          |
|------------|------------------------|-------------------------------------|
| VCC        | supply                 | ESP32 **3.3V** — see the 5V warning below |
| GND        | ground                 | ESP32 GND                           |
| SCL        | I2C clock              | GPIO22                              |
| SDA        | I2C data               | GPIO21                              |
| DRDY       | data-ready interrupt   | **leave unconnected** — the firmware polls |

GPIO21/GPIO22 are the ESP32 Arduino core's default `Wire` pins, which is
what `sense/magnetometer.cpp` gets by calling bare `Wire.begin()` with no
pin arguments. Neither pin is claimed by anything else in this firmware
(`main.cpp` uses 27 anemometer, 34 vane, 2/25/26 LEDs), so the default is
fine and no code change is needed for this wiring.

The QMC5883P has no address-select strap: `0x2C` is fixed. It is currently
the only device on the bus.

```
ESP32                         GY-271 board (QMC5883P)
                             ┌────────────────────────┐
3.3V ────────────────────────┤ VCC                    │
GND  ────────────────────────┤ GND                    │
GPIO21 (SDA) ────────────────┤ SDA   ← pull-ups here, │
GPIO22 (SCL) ────────────────┤ SCL     if the board   │
                             │ DRDY    has them (n/c) │
                             └────────────────────────┘
```

## Pull-up resistors — the part that matters

I2C is an **open-drain** bus: devices can only pull `SDA`/`SCL` *low*.
Nothing on the bus ever drives a line high; the pull-up resistors to VCC
do that. With no pull-up at all, both lines sit low forever, every
transaction NACKs, and `magnetometer.begin()` returns false — the serial
line reads `magnetometer init: FAILED (I2C error - check wiring/address)`.

**Most GY-271/GY-273 boards already have onboard pull-ups** (commonly
4.7 kΩ, sometimes 10 kΩ) wired from `SDA` and `SCL` to the module's `VCC`
pin. If yours does, **add nothing** — it is already correct.

**Check before soldering anything.** With the module unpowered and
disconnected, measure with a multimeter on the module's own header:

| Measurement       | Reading           | Meaning                          |
|-------------------|-------------------|----------------------------------|
| SDA ↔ VCC         | ~2–10 kΩ          | onboard pull-up present ✅        |
| SCL ↔ VCC         | ~2–10 kΩ          | onboard pull-up present ✅        |
| either, open/MΩ   | no continuity     | **missing — add external 4.7 kΩ** |

If they are missing, add one resistor per line, from the line to the
**3.3V** rail (not 5V):

```
3.3V ──┬──[ 4.7kΩ ]──┬── GPIO21 / SDA ── module SDA
       │             │
       └──[ 4.7kΩ ]──┼── GPIO22 / SCL ── module SCL
                     │
                (both lines pulled up to the same 3.3V rail
                 the module is powered from)
```

### Why 4.7 kΩ

The value is bounded from both sides:

- **Lower bound ≈ 1.1 kΩ.** I2C devices are only specified to sink 3 mA,
  so the pull-up must not demand more than that: 3.3 V / 3 mA ≈ 1.1 kΩ.
- **Upper bound ≈ 6 kΩ.** The pull-up and the bus capacitance form an RC
  charge curve; standard-mode (100 kHz) I2C allows a 1000 ns rise time,
  and t_r ≈ 0.85·R·C. For a realistic ~200 pF of board + short-cable
  capacitance that gives R_max ≈ 5.9 kΩ.

4.7 kΩ sits comfortably in the middle and is the conventional value for a
3.3 V bus. 2.2 kΩ is the right move if the run is longer or the bus is
sped up; 10 kΩ works on a very short bench jumper and gets marginal as
soon as cable is added.

### Why the ESP32's internal pull-ups are not enough

`Wire.begin()` on arduino-esp32 does enable the internal pull-ups on
GPIO21/22, so a bus with no external resistors at all sometimes appears to
work on a 10 cm jumper. Do not rely on it. The internal pull-ups are
nominally ~45 kΩ (spec range roughly 30–80 kΩ): at 45 kΩ even 100 pF of
capacitance gives a rise time of ~3.8 µs, nearly 4× the 1 µs that
100 kHz I2C allows. The failure mode is not a clean "doesn't work" — it is
intermittent NACKs and corrupt reads that look like a flaky sensor, which
in this firmware surfaces as sporadic `magnetometer=FAIL(using last-known
heading)` lines rather than an obvious wiring fault.

### Don't stack them blindly, but one extra is harmless

Pull-ups on the same bus are in parallel. Onboard 4.7 kΩ plus an external
4.7 kΩ gives 2.35 kΩ — still above the 1.1 kΩ floor, so it will work; it
is just unnecessary current. It matters only if more I2C devices, each
with its own onboard pull-ups, are ever added: three 4.7 kΩ boards in
parallel is 1.57 kΩ, and a fourth pushes the bus past what the chips can
sink. If devices are ever added, desolder the redundant onboard pull-ups
rather than accumulating them.

## ⚠️ Power the module from 3.3V, not 5V

**ESP32 GPIOs are not 5 V tolerant.** The module's pull-ups tie `SDA` and
`SCL` to whatever is on its `VCC` pin — so feeding the board 5 V pulls
both signal lines to 5 V and drives that straight into GPIO21/22. Some
GY-271 clones carry an onboard 3.3 V LDO and some do not, and the
silkscreen does not tell you which; the QMC5883P die itself runs at
2.0–3.6 V. There is no reason to take the risk: use the ESP32's 3.3V pin.
Current draw is a few hundred µA — no separate supply, regulator, or
level shifter is needed anywhere in this build.

## Cable length and placement

I2C is a **short-range, on-board bus**, not a cable bus. Keep the
magnetometer close to the ESP32 — practically, under ~1 m of plain wire at
100 kHz with 4.7 kΩ pull-ups, and use a twisted pair with GND for anything
past a few tens of centimetres. If the final enclosure ever needs the
magnetometer far from the ESP32, move the whole ESP32 enclosure rather
than extending the I2C bus; the fallbacks (2.2 kΩ pull-ups, shielded
twisted pair, or a P82B715 bus buffer) all add parts and failure modes for
a problem better solved by placement.

Placement has a second, magnetic constraint that outranks the electrical
one:

- Keep the sensor away from anything ferrous, any magnet, and any wire
  carrying meaningful current (including the ESP32's own supply leads and
  the WiFi radio's bursty draw). Those add a hard-iron offset that moves
  with the boat, which no calibration can remove.
- **A calibration is only valid for one rigid assembly.** The hard-iron
  offsets baked into `main.cpp`'s `kMagnetometerCalibration` (1713.5 /
  1984.0, from `scripts/qmc5883p-calibration.json`) were captured on the
  bench assembly at field range 8 G. Re-mounting the sensor, moving it
  inside the enclosure, or changing what sits next to it invalidates them
  — recalibrate per `scripts/README.md`. This is exactly why the physical
  mounting story (Epic 5, Story 5.2) is expected to end in a recalibration.
- The mount's **orientation** is also baked in: `up=-z, forward=+x` (the
  chip ends up upside down), verified with `--check-rotation`, which is
  why `magnetometer.cpp` negates the raw Y axis. Mounting it in a
  different orientation means re-running `--detect-up` and
  `--check-rotation` and revisiting that sign — see the comments in
  `sense/magnetometer.cpp` and `main.cpp`.

## Bring-up and troubleshooting

The firmware reports the magnetometer twice, both over serial:

- Once at boot: `magnetometer init: ok` / `FAILED (I2C error - check
  wiring/address)`.
- Every sample: `magnetometer=ok` with a `yawDeg` value, or
  `magnetometer=FAIL(using last-known heading)`.

| Symptom | Likely cause |
|---------|--------------|
| `init: FAILED` immediately | SDA/SCL swapped, module unpowered, GND not shared, or no pull-ups at all |
| Init ok, then intermittent `FAIL` | pull-ups too weak (internal-only) or cable too long/noisy |
| Nothing at `0x2C` when scanning | not a QMC5883P — scan the bus; `0x1E` = real HMC5883L, `0x0D` = QMC5883L, both need a different driver |
| Readings present but `yawDeg` stuck in a narrow band | **not a wiring fault** — uncalibrated hard-iron offset. See `scripts/README.md`: the bias is routinely larger than Earth's field, so `atan2` can only sweep an arc |
| Whole sampling loop freezes | was `bug-030`; `begin()` now sets `Wire.setTimeOut(1000)` so a stuck bus degrades to a failed read instead of a hang |

A bench sanity check before wiring to the ESP32 at all — the MCP2221
USB-I2C bridge and `uv run python -m gustik_scripts --bus N --help` — is
the fastest way to separate "bad module" from "bad ESP32 wiring", and the
bridge brings its own pull-ups so it tests the chip independently of this
question entirely.

## Status

The chip identity, register map, calibration, and mount handedness are all
confirmed on real hardware (bench, 2026-08-11, via the USB-I2C bridge),
and since 2026-08-12 the assembled station reports `magnetometer=ok` with
plausible `yawDeg` values that track physical rotation — so the pins and
levels documented above are working as wired today.

What is *not* measured is the pull-up situation specifically: the values
and bounds here are derived from the I2C specification and standard 3.3 V
practice, and the working board is presumed to carry onboard pull-ups
rather than confirmed to. Measure the module before adding any resistor,
and note that a bus running on internal pull-ups alone can work for a long
time before it doesn't. The calibration constants also still describe the
bench assembly, not the final boat-mounted enclosure — see `TODO.md` and
Story 5.2.
