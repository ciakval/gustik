# Mutual orientation: wind vane ↔ magnetometer

How the mast-head sensor array and the cabin magnetometer must be aligned
**to each other** so the reported wind direction is right. Wiring (which
RJ11 pin goes where, pull-ups, ADC values) is a separate concern and lives
in [`wind-sensor-wiring.md`](wind-sensor-wiring.md).

Short version, for the mounting described in this repo (vane arm to
starboard, anemometer arm to port):

> The sensor head's **0°/N reference points at the bow**. The magnetometer
> must therefore have its **+X axis pointing at the bow**, lying **level**,
> **component side down** (chip's +Z toward the keel), which puts **+Y to
> starboard**. Nothing else about where it sits matters, as long as it is
> bolted to the hull and away from iron and current.

The rest of this document is why, how to verify it, and what each way of
getting it wrong looks like in the data.

---

## 1. Why two sensors

The WH1080 wind vane on its own reports an **absolute** direction, but only
because the manufacturer's fixed-installation instructions have you point
one arm of the head at a cardinal direction (west) when you bolt it to a
shed. The vane's internal reference is then aligned with the world, and its
raw reading *is* the wind direction.

On a boat that instruction is deliberately broken. The station is on a
vessel that swings on its mooring, so:

- the **vane** is aligned to the **hull** (not the world), and reports where
  the wind comes from **relative to the boat**;
- the **magnetometer** reports where the **boat** points relative to
  magnetic north;
- the firmware adds them, recovering the world-relative direction on every
  sample:

```
absolute wind octant = (vane octant + yaw octant) mod 8
```

That is literally `correctWindDirectionOctant()` in
`firmware/src/correct/wind_direction.cpp`. Both terms are measured
clockwise-from-reference, viewed from above, which is what makes plain
addition correct.

**Corollary — the alignment rule.** The addition is only valid if the vane's
zero and the magnetometer's heading axis point *the same way*. They do not
have to be near each other; they have to be **parallel, same sense, both
level, and both rigidly fixed to the same hull**. That is the entire
requirement. Everything below follows from it.

---

## 2. What each side's "zero" actually is

### Vane

The vane's zero is electrical, not cosmetic: the 33 kΩ reed position (ADC
≈ 2943 on our unit) is octant 0, and octants increase **clockwise** in 45°
steps — see the measured 16-position table in
[`wind-sensor-wiring.md`](wind-sensor-wiring.md) and
`firmware/src/sense/vane_decode.cpp`. A wind vane reads the direction the
wind blows **from**, so octant 0 means "wind coming from whatever direction
the vane's 0° mark points at".

The anemometer arm has **no** orientation requirement — cups are
omnidirectional. It only matters here because it tells you how the head is
rotated: on this head the two arms are athwartships, vane on one end,
anemometer on the other.

### Magnetometer

`firmware/src/correct/wind_direction.cpp` computes `atan2(y, x)`, and
`firmware/src/sense/magnetometer.cpp` feeds it `x = forward` and
`y = left` (it negates the chip's raw Y to produce "left" — see the comment
there). The mount geometry is declared once, the same way the host-side
tooling declares it (`scripts/src/gustik_scripts/orientation.py`):

```
forward = +x     the axis whose bearing is reported
up      = -z     board is upside down
left    = up × forward = (-z) × (+x) = -y
```

So heading 0° means **the chip's +X axis points at magnetic north**, and
heading increases clockwise viewed from above.

---

## 3. The answer for this boat

Given: **sensor East → starboard**, i.e. the vane arm is on the starboard
side and the anemometer arm on the port side.

A compass rose runs N→E→S→W clockwise seen from above, so if E points to
starboard, **N points at the bow**. The vane's zero therefore points
forward, and the vane reports wind direction relative to the bow — exactly
what `wind_direction.h` says it assumes ("relative to the boat's bow").

The magnetometer must match it:

| Chip axis | Must point | Notes |
|-----------|------------|-------|
| **+X** | **bow** | this is the axis whose bearing becomes `yawDeg` |
| **+Y** | **starboard** | falls out of the other two; the firmware negates it to get "left" |
| **+Z** | **down**, toward the keel | board mounted **component side down** — this is the `up = -z` in the code |

Level, in the boat's normal floating attitude. Bolted down so it cannot
shift or rotate.

```
                    bow
                     ↑
                     │  +X
              ┌──────┴──────┐
   port  ←────┤             ├────→ starboard        +Z points DOWN
              │  QMC5883P   │  +Y                   (board upside down)
              └─────────────┘
                  (level)
```

If your breakout only silkscreens X and Y, +Z is the axis coming out of the
component side of the PCB — so "component side down" is the same statement.

### Worked examples

| Boat heading | Wind from | Vane octant | Yaw octant | Reported |
|--------------|-----------|------------:|-----------:|----------|
| north (bow N) | east (starboard beam) | 2 | 0 | 2 = **V** (east) |
| east (bow E) | east (dead ahead) | 0 | 2 | 2 = **V** (east) |
| west (bow W) | south (port beam) | 6 | 6 | 12 mod 8 = 4 = **J** (south) |

The middle row is the one worth internalising: the *same* wind reads as a
completely different vane octant depending on where the boat is pointing,
and the magnetometer is what cancels that out.

---

## 4. Failure modes

Every misalignment produces a **confident, stable, wrong** number — there is
no error signal for any of these. Know the signatures:

| Mistake | Signature in the data |
|---------|----------------------|
| Magnetometer +X to **stern** instead of bow | direction constantly wrong by **180°** (4 octants); looks plausible, is exactly backwards |
| Magnetometer rotated **90°** (e.g. +X to starboard) | constant 2-octant error |
| Magnetometer mounted **component side up** (+Z up, contradicting `up = -z`) | heading is **mirrored**: it runs *backwards* as the boat turns, so the error changes with heading instead of being constant. Swing the boat and watch `yawDeg` — if it decreases while you turn clockwise, this is it |
| Vane head rotated on its bracket | constant offset in whole octants, and it can change silently if the bracket slips |
| Magnetometer not level / boat heeled | error grows with heel; no tilt compensation exists (`wind_direction.h`) |
| Either device not rigid w.r.t. the hull | intermittent nonsense that correlates with nothing |

The mirrored case is the sneaky one, because a static check at one heading
can pass. Always verify by **turning through a full circle**, not by
checking a single direction.

---

## 5. Mounting the head (mast)

- **Align the head to the boat's centreline**, not to the world. The vane's
  0° mark to the bow, per section 3.
- **Lock it against rotation.** A head that can creep on its bracket
  silently rewrites the calibration. Mark the bracket and the head with a
  paint line so a slip is visible from the deck.
- **Watch out for a rotating mast.** On small dinghy rigs the mast can turn
  in its step. If the head is on such a mast, the vane's reference rotates
  with it and the yaw correction is wrong by however far the mast has
  turned — pin the mast, or mount the head on something fixed to the hull.
- Long cable to the head is fine: the anemometer is a reed switch and the
  vane is a resistor ladder. If the cable length changes, re-check the vane
  ADC anchors ([`wind-sensor-wiring.md`](wind-sensor-wiring.md) — the low
  end of the ladder is where it bites first).

## 6. Mounting the magnetometer (cabin)

The magnetometer stays near the ESP32 on purpose: it is I²C (SDA→GPIO21,
SCL→GPIO22), which does not survive a run up a mast. It does not need to be
near the vane — only parallel to it.

- **Level and rigid**, +X to the bow (section 3). Screwed or epoxied, not
  taped.
- **Away from iron and current.** Hard-iron calibration only cancels ferrous
  material that is *fixed relative to the sensor*. Keep clear of: the
  engine, batteries, the powerbank, speakers, any DC cable carrying real
  current, steel fasteners, and anything that gets moved (toolboxes,
  anchors, a spare battery). 20–30 cm from the ESP32 and its power leads is
  a reasonable minimum.
- **Anything you move later invalidates the calibration.** If the powerbank
  moves, recalibrate.

### Calibrating in place

Hard-iron offsets describe **one rigid assembly**, and the assembly that
matters is the finished boat installation — not the bench. Once the
magnetometer is in its final position with everything else aboard in its
normal place:

```sh
cd firmware && ~/.platformio/penv/bin/pio run -e mag_diag -t upload
cd ../scripts && PYTHONPATH=src python3 -m gustik_scripts.mag_calibrate   # 30 s, level
```

**Swing the boat through a full slow circle**, level, instead of tumbling
anything. `mag_calibrate`'s own help calls `--tumble` "the better default for
a fresh mount", and on the bench it is — but it cannot be done to a boat, and
it is not needed here: the firmware reads only X and Y (`readRawXY`), so a
level 360° turn captures exactly the two offsets that get used, with the
boat's own magnetic signature folded in. Then verify the mount before
trusting it:

```sh
PYTHONPATH=src python3 -m gustik_scripts.mag_calibrate --check-rotation --up=-z
```

It must report **clockwise**, not `MIRRORED`. `MIRRORED` means the board is
mounted the opposite way up from `up = -z` — flip the board, don't flip the
sign, since the firmware hard-codes this convention. If it prints
`!! NOT A ROTATION`, the swing was too small (see buglog bug-058).

The script prints paste-ready `mag.offsetX` / `mag.offsetY` lines for
`firmware/data/config.txt`, which is the preferred route — applying them is
`pio run -t uploadfs`, no reflash. Then put the station firmware back:

```sh
cd ../firmware && ~/.platformio/penv/bin/pio run -e esp32dev -t upload
```

---

## 7. Acceptance test (do this once, on the water or on the trailer)

The station firmware prints everything needed on Serial at 115200 baud, once
per sample cycle:

```
[12345ms] sensors: pulses=0 windSpeedMs=0.00 vaneOctant=0 magnetometer=ok yawDeg=0.0 windDirOctant=0
```

Check the three claims separately — never all at once, or a double error can
cancel and pass:

1. **Vane zero is at the bow.** Hold the vane pointing at the bow.
   → `vaneOctant=0`. Point it at starboard → `vaneOctant=2`. If instead you
   get 4 and 6, the head is 180° out; if 6 and 0, it is 90° out.
2. **Magnetometer zero is at the bow.** Point the bow at magnetic north
   (hand compass, or a phone held well away from the boat).
   → `yawDeg≈0`. Point the bow east → `yawDeg≈90`. If it reads ≈270 for east,
   the mount is mirrored (section 4).
3. **The sum works.** Point the bow east and hold the vane at the bow.
   → `windDirOctant=2`. Then, without moving the vane relative to the boat,
   swing the bow to south: `vaneOctant` stays 0, `yawDeg` goes to ≈180, and
   `windDirOctant` must follow to 4. **The reported direction tracking the
   boat's rotation while the vane is fixed to the boat is the whole point of
   the correction** — if `windDirOctant` stays put, the yaw correction is not
   doing anything.

Step 3 is the real test. Steps 1 and 2 exist so that when it fails you know
which half is wrong.

---

## 8. Notes and known limits

- **North here is magnetic north.** Declination in Czechia is roughly
  +5.5° E (2026), well inside the ±22.5° an octant already spans, so the
  firmware ignores it deliberately. The host tooling can apply it
  (`--declination 5.5`) for bench readings in degrees; the station never
  reports degrees at all (AD-5).
- **No tilt compensation.** The chip has no accelerometer, so heel and pitch
  cannot be corrected from its readings. Acceptable for a boat that is
  anchored and roughly upright; error grows with heel.
- **There is no configurable mount offset between the two frames.** The
  firmware assumes the vane's zero and the magnetometer's +X are the same
  direction, and there is no constant to say otherwise. If a future
  installation cannot align them physically, that offset has to be added in
  code (a whole-octant constant folded into `correctWindDirectionOctant`) —
  do not "compensate" by rotating the hard-iron numbers, which are a
  different quantity and will not work.
- **Both calibrations are per-installation.** The vane ADC anchors depend on
  the pull-up and cable; the hard-iron offsets depend on the boat. Moving the
  station to a different vessel means redoing both.
