# Wind sensor (WH1080/WH1090) → ESP32 wiring

The salvaged WH1080/WH1090 wind sensor head combines the anemometer (cup
wheel) and the wind vane in one unit, sharing a single 4-wire cable
terminated in an RJ11 plug. Both circuits are fully passive — reed
switches and resistors only, no active electronics, no power pins on the
sensor side.

Source: Shenzhen Fine Offset Electronics Co., Ltd's own datasheet for this
exact sensor kit (the manufacturer of the WH1080/WH1090 station), which
gives the pin assignment and the wind vane's resistance table directly.
Cross-checked against independent community reverse-engineering (Cumulus
forums, Raspberry Pi forums) - both agree: inner pins = anemometer, outer
pins = wind vane.

## RJ11 pinout

Looking at the plug with the clip on the underside and the contacts facing
you, cable exiting downward, pins numbered 1-4 left to right:

```
 ___________
|  1 2 3 4  |   <- gold contacts (pin 1 = leftmost)
|___________|
      |
    cable
```

| RJ11 pin | Circuit          | Notes                                    |
|----------|------------------|-------------------------------------------|
| 1        | Wind vane        | outer pair - either wire, no polarity      |
| 2        | Anemometer       | inner pair - either wire, no polarity      |
| 3        | Anemometer       | inner pair - either wire, no polarity      |
| 4        | Wind vane        | outer pair - either wire, no polarity      |

Both circuits are symmetrical (a plain switch, a plain 2-wire variable
resistor) - the two wires of each pair are interchangeable, and the
connector can go in "backwards" without effect. Only inner-vs-outer
(anemometer vs vane) matters.

## Wiring to the ESP32

Matches the pins already assumed by `firmware/src/sense/anemometer.h`
(`kAnemometerPin`) and `firmware/src/sense/vane.h` (`kVanePin`), both set
in `firmware/src/main.cpp`.

**Anemometer (RJ11 pins 2 & 3) → GPIO27:**
- one wire → GPIO27
- other wire → GND
- No external resistor needed. `anemometer.cpp` already configures
  `INPUT_PULLUP` (ESP32's internal pull-up, ~45kΩ) - the reed switch just
  pulls the pin to GND once per rotation; the ISR counts `FALLING` edges.

**Wind vane (RJ11 pins 1 & 4) → GPIO34:**
- **External pull-up resistor required - this one is not optional.**
  GPIO34 is an ESP32 input-only ADC pin (ADC1_CH6); unlike GPIO27, pins
  34-39 have no internal pull-up/pull-down hardware at all, so
  `INPUT_PULLUP` cannot help here.
- Wiring: `3.3V --[10kΩ resistor]-- GPIO34 --(vane, either wire)--(other wire)-- GND`
- 10kΩ is the manufacturer's own reference design value (their datasheet's
  example circuit uses a 5V supply with a 10kΩ pull-up; the ESP32's 3.3V
  rail works with the same resistor value, it just scales the output
  voltage range down proportionally - see the ADC counts below).

```
3.3V ──[ 10kΩ ]──┬── GPIO34 (ADC1_CH6)
                  │
              RJ11 pin 1 or 4
                  │
              (vane's internal
             variable resistance)
                  │
              RJ11 pin 4 or 1
                  │
                 GND
```

## Wind vane resistance table

From Fine Offset's datasheet - the vane has 8 reed switches, each wired to
a different resistor; adjacent switches can close together for 16
positions total (two resistors in parallel). `vane.cpp` only uses the 8
primary octants (`kOctantAdcReadings`), so only every other row matters
for this firmware:

| Direction | Resistance | ADC count (12-bit, 3.3V supply, 10kΩ pull-up) |
|-----------|-----------:|-----------------------------------------------:|
| 0°  (N)   | 33 kΩ      | ~3143 |
| 45°       | 8.2 kΩ     | ~1845 |
| 90°       | 1 kΩ       | ~372  |
| 135°      | 2.2 kΩ     | ~738  |
| 180° (S)  | 3.9 kΩ     | ~1149 |
| 225°      | 16 kΩ      | ~2520 |
| 270°      | 120 kΩ     | ~3780 |
| 315°      | 64.9 kΩ    | ~3548 |

(16-position intermediate values - 22.5°=6.57kΩ, 67.5°=891Ω, 112.5°=688Ω,
157.5°=1.41kΩ, 202.5°=3.14kΩ, 247.5°=14.12kΩ, 292.5°=42.12kΩ,
337.5°=21.88kΩ - not currently used by this firmware's 8-octant model, but
recorded here in case 16-position resolution is ever wanted.)

The ADC counts above are a computed sanity check
(`3.3 * R / (R + 10000) / 3.3 * 4095`), not a substitute for measuring the
real hardware - `vane.cpp`'s `kOctantAdcReadings` is still a placeholder
flagged for real-hardware calibration (see `TODO.md`). The computed values
are in the same range as that placeholder table, which is a good sign the
10kΩ assumption already baked into it is correct, but actual mounted
resistances, cable length, and ADC nonlinearity near the rails will shift
the real numbers - measure with a multimeter (resistance between pins 1
and 4 at each of the 8 positions) or directly read raw ADC counts once
wired up, and update `kOctantAdcReadings` accordingly.

## Measured values (2026-08-15, real hardware)

Measured on the real ESP32 + real vane, 10kΩ pull-up, 3.3V rail, from an
18337-sample raw serial capture of two full hand rotations pausing at each
detent. These supersede the computed ADC counts in the table above for
calibration purposes — the computed column stays as the sanity check it
always was.

Method: the bring-up firmware printed nothing but `mV=... adc=...`; all
clustering and decoding was done afterwards on the host. That split is the
project's standing convention for sensor bring-up (see `.wolf/cerebrum.md`
Decision Log) and it is what made the half-detent finding below possible —
the raw capture could be re-questioned weeks-of-thinking later without
touching the hardware.

### The 8 primary octants

| Octant | Measured Ω | Datasheet Ω | **Measured ADC** | settled samples | ADC spread |
|--------|-----------:|------------:|-----------------:|----------------:|-----------:|
| 0°  (S sever)  |  35 361 | 33 000  | **2943** | 2364 | 3 |
| 45° (SV)       |   8 585 |  8 200  | **1663** | 1861 | 4 |
| 90° (V)        |   1 059 |  1 000  | ** 209** | 2028 | 2 |
| 135° (JV)      |   2 302 |  2 200  | ** 572** | 1706 | 3 |
| 180° (J jih)   |   4 052 |  3 900  | ** 974** | 2634 | 3 |
| 225° (JZ)      |  16 751 | 16 000  | **2315** | 2932 | 6 |
| 270° (Z zapad) | 127 929 | 120 000 | **3855** | 2445 | 4 |
| 315° (SZ)      |  68 678 | 64 900  | **3465** | 1952 | 5 |

Two independent captures agree to **within 1 ADC count on every octant**, and
a third (this one) reproduces all eight to within 4 counts. The closest pair
of adjacent primaries is ~360 counts apart, so nearest-match has wide margin.

### The 8 intermediate half-detents — and why they matter

The vane has 8 reed switches but **16 detents**: at the half-way positions two
adjacent switches close together, putting their resistors in *parallel*.
Parallel resistance is always *lower* than either leg, so a half-detent does
**not** read between the two octants it sits between — it lands somewhere
unrelated on the ladder.

An earlier revision of this document called that "expected, not a fault",
reasoning that intermediates would resolve to whichever primary was nearer.
**That was wrong** (buglog bug-050). With only the 8 primary anchors in the
table, three half-detents decoded to a badly wrong octant:

| True position | ADC | Decoded as (8 anchors) | Error |
|---------------|----:|------------------------|------:|
| 157.5° | 339  | 90°  | 67.5° |
| 292.5° | 3134 | 0°   | 67.5° |
| 337.5° | 2604 | 225° | **112.5°** |

These are mechanically stable rest positions — a steady wind parking the vane
on one produced a confident, stable, badly wrong direction with no error
signal anywhere.

All 16 measured positions, as now encoded in `firmware/src/sense/vane_decode.cpp`:

| Position | ADC | Decodes to | | Position | ADC | Decodes to |
|---------:|----:|-----------:|-|---------:|----:|-----------:|
| 0°     | 2943 | 0°   | | 180°   |  974 | 180° |
| 22.5°  | 1442 | 45°  | | 202.5° |  803 | 225° |
| 45°    | 1663 | 45°  | | 225°   | 2315 | 225° |
| 67.5°  |  175 | 90°  | | 247.5° | 2195 | 270° |
| 90°    |  209 | 90°  | | 270°   | 3855 | 270° |
| 112.5° |  107 | 135° | | 292.5° | 3134 | 315° |
| 135°   |  572 | 135° | | 315°   | 3465 | 315° |
| 157.5° |  339 | 180° | | 337.5° | 2604 | 0°   |

A half-detent is exactly between two octants, so which one it reports is a
convention, not a measurement: each is assigned to the **higher** octant, so
octant *k* covers the arc (45k − 22.5°, 45k]. Worst-case error is now 22.5°,
which is the floor for an 8-octant output (AD-5). Replaying the full capture
through the 16-anchor table changes 309 of 18337 samples (1.7%) and **zero
primary-detent samples** — the confirmed primary calibration is untouched.

Note the ADC's severe nonlinearity at the bottom of its range: 90° (1059Ω)
reads 209 counts where an ideal divider predicts 392. The three lowest
positions (112.5°, 67.5°, 90° at 107 / 175 / 209) are compressed into ~100
counts. They still separate cleanly, but this is the one region where a
changed pull-up or a long cable run would bite first.

### Other findings from this run

1. **The vane is the WH1080 ladder, correctly on the RJ11 outer pair, with a
   working pull-up.** Sixteen distinct levels, right order, right spacing.
2. **The error is systematic, not random.** Every position measures ~4-6%
   above its datasheet resistance (always positive) — a scale factor from
   pull-up tolerance, rail voltage, and ADC calibration, not a contact or
   wiring fault. Exactly why the table is measured rather than computed.
3. **The readings are stable over time.** A separate 100 s capture parked at
   225° gave mean ADC 2315.1 across 159 settled samples (min 2311, max 2319),
   against 2315.6 from the rotation capture — 0.5 counts of cross-capture
   drift.
4. 270° (120kΩ) reads 3061 mV, near but not at the ESP32's ~3.1V saturation
   at 11dB attenuation, and stays cleanly separated from 315°. **The 10kΩ
   pull-up stays** — no need to drop to 4.7kΩ.

**Re-measure if the pull-up value, the cable run, or the sensor unit changes**
— all three shift the divider. This is a correctness input, not a tuning
knob: readings map to the *nearest* anchor, so a bad value does not degrade
into "slightly wrong degrees", it maps real positions onto unrelated octants.
That is how both bug-045 and bug-050 happened. `firmware/test/test_vane_decode/`
pins the whole table, including all three bug-050 regressions.

### If repeating the capture

The bring-up firmware is deliberately not kept in the tree — it is ~20 lines
and writing it fresh costs less than maintaining it:

```cpp
#include <Arduino.h>
static const uint8_t kVanePin = 34;
void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
    analogSetPinAttenuation(kVanePin, ADC_11db); // full 0-3.3V range
    pinMode(kVanePin, INPUT);                    // input-only pin, no internal pull
}
void loop() {
    Serial.printf("READING: mV=%u adc=%u\n",
                  analogReadMilliVolts(kVanePin), analogRead(kVanePin));
    delay(50);
}
```

Give it its own PlatformIO env (`build_src_filter = -<*> +<the_file.cpp>`) so
its `setup()`/`loop()` never link alongside `main.cpp`. Read `mV` and `adc`
in one line each if you want them paired — they are two separate conversions,
so the two numbers on a line are *not* the same sample.

Capture gotchas: opening `/dev/ttyUSB0` produces a burst of duplicated or
truncated line fragments before the stream settles, so parse defensively
rather than assuming the capture failed. Make sure no earlier
`cat /dev/ttyUSB0` is still alive — two readers on one tty split the byte
stream, and that loses data for good.

## Power: does the ESP32 alone suffice?

**Yes.** Per the manufacturer's own datasheet: "These sensors contain no
active electronics, instead using sealed magnetic reed switches and
magnets to take measurements. A voltage must be supplied to each
instrument to produce an output." That voltage requirement is exactly the
pull-up circuitry described above (internal for the anemometer, external
10kΩ for the vane) - it is not a separate power feed to the sensor head
itself. The ESP32's own 3.3V rail is sufficient for both circuits; no
external power supply, battery, or regulator is needed for the wind
sensor unit.

The only thing to keep in mind for the real installation: cable run length
from the masthead sensor down to the ESP32 adds resistance and pickup
noise to both circuits. The anemometer's ISR-based pulse counting is
tolerant of that; the wind vane's 10kΩ divider is more sensitive to added
series resistance on a long run - if octant readings look inconsistent
once mounted at the final cable length, that is the first thing to
re-check (shorter/shielded cable, or a lower pull-up value to raise the
signal current, before assuming a wiring or calibration error).
