# Status LED panel — wiring

**Written 2026-08-16**, when the firmware side (rollout phases 1 and 2) was
implemented. The design, the reasoning and the full encoding live in
`docs/superpowers/specs/2026-08-16-status-led-panel-design.md`; this file is
the bench reference for *building* the thing.

**Nothing here has been built yet.** The firmware drives these pins as of
this commit, but no LED has been soldered — see §7.

---

## 1. What it is

Nine LEDs in one flat row plus one button, on the same breadboard as the
ESP32. Two groups, and the split is the whole design:

```
  R   Y   G   B            G   G   Y   Y   R
 └───── status ─────┘     └────── detail ──────┘
   four indicators           a green→red ramp
   colour = identity         colour = severity
   ALWAYS ON                 button selects what it shows
```

Left group never changes, so going to look at the wind can never hide a
fault. Right group shows **exactly one lit dot**, sliding left→right as
things get worse; it defaults to **wind speed**, and the button cycles it
through signal and sensors.

Two reds at the extreme opposite ends of the row, and blue appears exactly
once — blue only ever means "the loop is alive".

---

## 2. Pin map

The board is a 38-pin **ESP32 DevKit v1 / NodeMCU-32S**-style module
(ESP32-D0WD-V3, 4 MB flash, CP2102) — probed over serial, not a DevKitC.

| Signal | GPIO | Column | Notes |
|---|---:|---|---|
| status RED (fault) | 32 | left | |
| status YELLOW (radio) | 33 | left | |
| status GREEN (data landing) | 25 | left | **was** `config loaded` |
| status BLUE (life) | 26 | left | **was** `WiFi connected` |
| detail 1 GREEN | 19 | right | |
| detail 2 GREEN | 18 | right | |
| detail 3 YELLOW | 17 | right | GPIO**5** sits physically between 18 and 17 and is **skipped** |
| detail 4 YELLOW | 16 | right | |
| detail 5 RED | 4 | right | |
| button | 13 | left | `INPUT_PULLUP`, other side to GND |
| onboard LED | 2 | — | **unchanged**, Story 2.4 / FR-5 |

Every one of these is overridable from `platformio.ini` without touching
source — see `firmware/src/indicate/hw/panel_pins.h`:

```ini
build_flags = -DGUSTIK_PANEL_PIN_STATUS_RED=32
```

GPIO25/26 are *taken over* from the 2026-08-11 bring-up diagnostics. No LED
was ever fitted to GPIO2, 25 or 26 and nobody ever read one, so this is a
reassignment, not a migration. GPIO2 is deliberately left alone, which keeps
FR-5's disconnect signal satisfied with the panel absent, disabled or asleep.

Deliberately avoided: **GPIO0/2/5/15** (strapping), **GPIO12** (must read low
at boot), **GPIO34–39** (input only), **GPIO6–11** (see the warning below).
GPIO13 was chosen for the button because it is not a strapping pin and has no
boot-time output glitch — **GPIO14 does**, which would flash an LED at every
reset.

> ### ⚠️ GPIO6–11 are physically exposed on this board
>
> `CLK / SD0 / SD1 / SD2 / SD3 / CMD` at the bottom of both columns are the
> **SPI flash bus**. They look like six free pins and they are not: anything
> connected there crashes the chip or corrupts flash.
>
> - **`V5` is immediately adjacent to `CMD` (GPIO11)** — and `V5` is exactly
>   where the battery pack's `+` wire lands. One stray strand between them
>   puts 6.4 V onto a flash pin.
> - **The button on GPIO13 is four pins above `V5`**, with `SD2/SD3/CMD` in
>   between. Route that wire away from the bottom of the header, not along it.

---

## 3. Resistors — 330 Ω on all nine

Active high: `GPIO → R → LED anode`, `cathode → GND`, so
`digitalWrite(pin, HIGH)` lights it.

330 Ω is the only value on hand and all four colours light visibly with it.
One value everywhere also means there is no chance of fitting the wrong
resistor to the wrong position, which is worth more here than an optimal
per-colour match.

| Colour | Vf | I at 330 Ω from 3.3 V |
|---|---|---|
| Red / yellow / green | 2.0–2.4 V (kit's note) | 2.7–3.9 mA |
| Blue | 2.6 V (measured) | 2.1 mA |

`I = (3.3 V − Vf) / 330 Ω`. All far inside the ESP32's 12 mA recommended
per-pin figure (40 mA absolute max). Worst case for the whole panel is four
status lanes solid plus one detail dot ≈ **15 mA**.

Colour count, inside the "up to 5 of each" ceiling:

| | red | yellow | green | blue | total |
|---|---:|---:|---:|---:|---:|
| status | 1 | 1 | 1 | 1 | 4 |
| detail | 1 | 2 | 2 | 0 | 5 |
| **total** | **2** | **3** | **3** | **1** | **9** |

The bright-green-InGaN trap (Vf 2.9–3.3 V, barely lights from 3.3 V) **does
not apply to this kit**. Keep it in mind only if a lane is replaced from a
different batch — the symptom is one colour visibly dimmer than the rest at
the same resistor, and the check is an LED in series with 1 kΩ across 3.3 V,
measuring across the LED itself.

Still unverified: **direct sunlight**. 2–4 mA is fine in shade, and v1 lives
in a cardboard box which is shade by construction. Prefer shading the panel
over raising current; 220 Ω (4–6 mA) is the fallback if a lane needs punch.

---

## 4. Wiring

```
      LEFT COLUMN (EN/RST side)                RIGHT COLUMN
   ┌──────────────────────────────┐    ┌──────────────────────────────┐
   │ 34 ── vane        (existing) │    │ 21 ── SDA         (existing) │
   │ 35 ── battery sense (phase 6)│    │ GND ─────────────────┐       │
   │ 32 ──[330Ω]──▶|──┐  RED      │    │ 19 ──[330Ω]──▶|──┐   │ GRN 1 │
   │ 33 ──[330Ω]──▶|──┤  YELLOW   │    │ 18 ──[330Ω]──▶|──┤   │ GRN 2 │
   │ 25 ──[330Ω]──▶|──┤  GREEN    │    │  5 ── SKIP! strapping pin    │
   │ 26 ──[330Ω]──▶|──┤  BLUE     │    │ 17 ──[330Ω]──▶|──┤   │ YEL 3 │
   │ 27 ── anemometer (existing)  │    │ 16 ──[330Ω]──▶|──┤   │ YEL 4 │
   │ 14                │          │    │  4 ──[330Ω]──▶|──┤   │ RED 5 │
   │ 12                │          │    │  0 ── SKIP! strapping pin    │
   │ GND ──────────────┘ common   │    │  2 ── onboard LED (existing) │
   │ 13 ──┐              cathode  │    │ 15 ── SKIP! strapping pin    │
   │ SD2  │  ⚠ flash bus          │    │ SD1  ⚠ flash bus             │
   │ SD3  │  ⚠ do not use         │    │ SD0  ⚠ do not use            │
   │ CMD  │  ⚠                    │    │ CLK  ⚠                       │
   │ V5 ──┼── battery pack +      │    │                              │
   └──────┼───────────────────────┘    └──────────────────────────────┘
          │
     ┌────┴────┐
     │  push   │   momentary, normally open
     │ button  │   GPIO13 → button → GND
     └────┬────┘   internal pull-up, active LOW
         GND

   ▶|  = LED, arrow points from anode (GPIO side) to cathode (GND side)
```

Physical arrangement is **one flat row of nine** in the order
`R Y G B ⎢ G G Y Y R`, with a visible gap between the groups. The row's order
is a wiring choice, not a pin-order constraint — jumpers can cross freely,
and the ZY-204 breadboard (64 rows) has room to spare.

**No button on hand yet?** Poke a bare jumper into GPIO13's row, leave the
far end loose, and tap it against the ground rail. The 30 ms debounce handles
the bounce and it is electrically identical to a momentary switch — enough to
exercise every mode on the bench.

---

## 5. Fitting fewer than nine is supported

A tested invariant, not a hope: driving a GPIO with nothing attached is
harmless, and an unwired `INPUT_PULLUP` button reads "released" forever. No
code change in any of these cases.

Priority order if you are short of parts or time:

1. **status RED** — faults; the only lane that says something is wrong
2. **status BLUE** — alive/heartbeat; the bug-030 hung-loop detector
3. **status GREEN** — data is actually landing
4. **status YELLOW** — radio
5. the detail group

With the status group alone you still get every fault code and proof of life;
the button simply has nothing to show. With the detail group partly fitted,
unfitted positions are dark and the dot is still readable by position.

---

## 6. Checking the wiring

Nine LEDs means nine chances to swap two jumpers, so there is a bring-up
sketch that does nothing but walk them in physical row order:

```sh
cd firmware
~/.platformio/penv/bin/pio run -e panel_diag -t upload
~/.platformio/penv/bin/pio device monitor -b 115200
```

What to look for:

- **Exactly one LED lit at a time, moving strictly left to right**, then
  repeating. Any LED out of order is two swapped jumpers.
- **Every LED lights, at roughly the same brightness.** One visibly dimmer
  lane at the same resistor means a higher-Vf part — see §3.
- **Colours in the order `R Y G B | G G Y Y R`.**
- **`BTN <micros> <level>` lines** on press and release; level 0 is pressed.
  A burst of them within a few milliseconds is contact bounce, which is
  exactly what the firmware's 30 ms debounce swallows.

It proves the *wiring*. The panel's logic is pure and already under test on
the host (`pio test -e native`, `test_panel_*` and `test_button`).

Put the station firmware back afterwards:

```sh
~/.platformio/penv/bin/pio run -e esp32dev -t upload
```

---

## 7. Order of work

| Step | Why it is where it is |
|---|---|
| Solder the nine LEDs + resistors and the button | |
| `pio run -e panel_diag -t upload`, check §6 by eye | Before trusting anything the station firmware says about them |
| `pio run -e esp32dev -t upload` | The panel is on by default; nothing needs enabling |
| **Redo the magnetometer hard-iron calibration** | **Not optional and not reorderable.** See below |

**The calibration ordering is a hard rule.** Hard-iron offsets describe the
*whole assembly*, and the board is permanently mounted on a steel plate. The
panel adds nine LED lanes and a button to that assembly, so
`python3 -m gustik_scripts.mag_calibrate` must run **after** the panel, the
button and (later) the battery divider are in their final positions, and
nothing may be rerouted afterwards without redoing it. Calibrating before the
panel is fitted produces numbers describing an assembly that no longer
exists — and nothing in the firmware or the dashboard would show they are
wrong.

The panel's own currents are *not* the reason. They were checked and are
irrelevant: one LED at 3 cm is ~20 nT against Earth's ~50 µT, roughly 0.02°
against a 22.5° octant; even the supply current at 120 mA and 2 cm is ~1.4°,
and it is a steady DC offset that hard-iron calibration absorbs anyway
(twisting the pack's `+` and `−` leads together cancels most of it for free).
The reason is simply that *any* rewiring invalidates the offsets.

**Soft iron is the part calibration will not fix.** A steel plate does not
just add a fixed offset, it distorts the field direction-dependently. Cheap
check with no new code: after calibrating in place, look at the captured XY
scatter — visibly elliptical rather than circular means soft iron worth
correcting.

---

## 8. Turning it off

Three independent ways, any one sufficient:

| Level | How | Effect |
|---|---|---|
| build | `-DGUSTIK_STATUS_PANEL=0` in `platformio.ini` | The panel is not compiled at all; GPIO25/26 revert to `config loaded` / `WiFi connected` |
| config | `leds.enabled=false` in `config.txt`, then `pio run -t uploadfs` | All nine pins stay low from boot, button ignored. **The durable way** |
| runtime | long-press the button (≥ 2 s) | Everything dark until the next power cycle — never persisted, on purpose |
| wiring | do not solder anything | Behaves exactly as the Station did before this change |

`leds.timeoutSeconds` (default 300, `0` = never) is how long with no button
press before the panel sleeps to a blue heartbeat plus the red fault code.
Sleeping keeps "the whole row dark means not running" true; hard-off
deliberately breaks it, which is why hard-off is runtime-only.

---

## 9. Not built yet

- **Battery sense (phase 6)** — a 3 × 10 kΩ divider to GPIO35 behind
  `-DGUSTIK_PANEL_BATTERY=1`, a fifth detail mode, and an all-nine-lanes
  low-battery alarm. Fully specified in §4.7/§5.9/§5.10 of the design doc,
  deliberately deferred because it is the only part needing hardware beyond
  LEDs and a button.
- **The Czech section in `manual.html`** — the lane table, the fault codes and
  the button gestures, for the person actually on the boat.
- **The legend where the LEDs are.** The fault codes are useless if they only
  exist on a web page you cannot reach while code 3 is flashing. For v1 that
  means **marker pen on the cardboard box**, which cannot be lost and costs
  nothing.
