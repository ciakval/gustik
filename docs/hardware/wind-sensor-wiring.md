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
