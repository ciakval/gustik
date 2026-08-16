# Status LED panel + mode button — design

**Date:** 2026-08-16 · **Status:** design only, nothing implemented ·
**Scope:** `firmware/` (plus one Czech section in `backend/src/static/manual.html`)

Once the Station is on the committee boat there is no serial console, no
laptop, and often no phone signal. Today the only on-device feedback is three
LEDs whose meanings are `config parsed` (GPIO25), `WiFi associated` (GPIO26)
and `connection unhealthy` (GPIO2, the onboard one, Story 2.4 / FR‑5). That
answers roughly two of the fifteen questions someone standing on a rocking
boat would actually ask, and two of the three LEDs end up invisible inside a
sealed enclosure anyway.

This document designs a **4‑LED status panel plus one button**, entirely
optional at build time, at config time and at wiring time, that answers those
questions without a computer — and that costs zero flash when switched off.

---

## 1. Constraints and non‑goals

**Constraints**

| # | Constraint | Consequence in this design |
|---|---|---|
| C1 | Flash is nearly full: **92.4 %** used, ~98 KB free (measured today, §9) | Whole feature behind a build flag; blink engine is a table, not code per state; no new format strings; a partition‑table escape valve is specified |
| C2 | `loop()` must never block or be delayed | Panel is a non‑blocking tick, no `delay()`, no ISR, no allocation |
| C3 | The panel must never affect measurement or transmission | One‑way data flow: the panel **reads** a snapshot, never writes to anything else (§8) |
| C4 | Hardware may be absent | Missing LEDs and a missing button must both be silently harmless (§7) |
| C5 | Supply is **4 × AA (~3000 mAh)**, not a powerbank — powerbanks auto‑shut‑down after ~2 h even in "low output" mode | Auto‑sleep now has a real (if modest) battery justification, not only a distraction one; budget re‑done against 3000 mAh in §4.6 |
| C6 | LEDs are a distraction on a boat, especially at dusk | Auto‑sleep to a heartbeat; a long press kills them entirely |
| C7 | Kit LEDs (red/yellow/green/blue, **Vf 2.0–2.4 V** per their note; blue measured 2.6 V) and **330 Ω is the only resistor value on hand** | One resistor value throughout at ~2–4 mA per lane — confirmed visible on all four colours (§4.2) |
| C8 | **No enclosure exists.** First deployment will be a cardboard box or similar, and an operator can simply peek inside at any LED | v1 needs no penetrations, no light pipes, no waterproofing — and the box shades the panel, which largely retires the daylight worry. §4.5 keeps the sealed‑enclosure guidance for Story 5.2 only |
| C9 | The three LEDs already in the firmware (GPIO2/25/26) have **never been fitted or observed** | Those pins are free to reassign — no migration, no compatibility concern, nothing depending on their present meaning (§4.1) |

**Non‑goals**

- Not a replacement for the dashboard or `/status.html`. Anything needing
  numbers, history, or more than one fact at a time belongs there.
- No display, no buzzer, no radio beacon, no OTA trigger on the button.
- No LED‑driven configuration (no "hold to enter setup"). Config stays
  `config.txt` + `uploadfs`.
- Not a safety instrument. Wind‑speed indication (§5.6) is a coarse
  situational cue; the dashboard's existing disclaimer still governs.

---

## 2. What a person on the boat actually needs to know

Written as the questions someone asks, in the order they ask them, with the
failure each one catches. This inventory is the real content of the design —
the encoding in §5 is just how it gets onto four LEDs.

| # | Question | Catches | Currently visible? |
|---|---|---|---|
| Q1 | Is it powered and running at all? | Powerbank cut out (low‑draw auto‑shutoff), loose USB, brown‑out | ❌ |
| Q2 | Is the firmware **loop** alive, or hung? | The bug‑030 class: an I2C/HTTP call blocking forever. Serial went silent; nothing on the boat changed | ❌ |
| Q3 | Did it find its config? | `config.txt` missing / wrong filesystem image / never uploaded after a partition change | ⚠️ GPIO25, invisible in an enclosure |
| Q4 | Is Wi‑Fi associated? | Wrong password, AP off, out of range | ⚠️ GPIO26, same |
| Q5 | **Which** network — shore or the phone hotspot? | Silently draining someone's phone all day; or being on the wrong AP with no route out | ❌ |
| Q6 | How strong is the signal? | The boat drifting out of range; deciding where to anchor or where to put the AP. **Actionable in real time** | ❌ |
| Q7 | Is the backend reachable? | DNS/route/TLS failure, backend down, wrong URL — distinct from Q4 | ⚠️ conflated into GPIO2 |
| Q8 | Is the token accepted? | 401 after a token rotation | ❌ |
| Q9 | Is data actually being **stored**? | The bug‑031 class: HTTP 2xx, `inserted = 0`, dashboard frozen while the device reports success | ❌ |
| Q10 | Is there a backlog buffered? | An outage in progress, or a backfill that never drains | ❌ |
| Q11 | Is the clock synced? | NTP never succeeded → timestamps wrong → history unusable | ❌ |
| Q12 | Is the anemometer wired and turning? | **bug‑059 verbatim**: a faulty wire produced `pulses=0` forever, indistinguishable from calm without a serial console | ❌ |
| Q13 | Is the vane wired? | Open/short on GPIO34 lands the ADC outside every known detent band | ❌ |
| Q14 | Is the magnetometer answering? | I2C failure → headings silently freeze at the last known value | ❌ |
| Q15 | What is the wind doing, right now, with no phone? | The whole point of the station, when the network is the thing that is broken | ❌ |
| Q16 | **How much battery is left?** | The station going dark mid‑regatta. New with C5: a powerbank at least had its own charge LEDs, **4 × AA has no gauge of any kind** | ❌ — answered by phase 6, §4.7 |

Two observations shape everything below:

- **Q2 is the highest‑value item and it is free.** A heartbeat LED makes
  "hung" distinguishable from "unpowered" at a glance — the one symptom
  bug‑030 conspicuously did not produce.
- **Q6, Q12 and Q15 are the only ones that are *actionable while standing
  there*.** They justify the button: they need more resolution than a
  binary lamp, but only occasionally.

---

## 3. Design shape

Four LEDs cannot show fifteen facts at once, so the panel is layered:

1. **A default "at a glance" mode** with four independent lanes, each a fixed
   colour with a fixed meaning, readable without counting anything. It answers
   Q1–Q4, Q7, Q9, Q10 immediately, and points at the rest.
2. **A single fault code** on the red lane (N flashes, pause, repeat) that
   names *which* problem, for the cases where "something is wrong" is not
   enough. Q3, Q7, Q8, Q9, Q11 collapse into this.
3. **Three detail modes** reached by pressing the button, each re‑purposing all
   four LEDs for one quantity: signal strength (Q5, Q6), sensor liveness
   (Q12–Q14), wind (Q15).
4. **Sleep**, so that none of this blinks at anybody for eight hours.

The button is only ever needed for layer 3. Layers 1, 2 and 4 work with no
button fitted at all.

### Reading rules (learn once, apply everywhere)

| Pattern | Means |
|---|---|
| **Solid** | good / present / confirmed |
| **Slow blink** (1 Hz) | attention — working, but degraded |
| **Fast blink** (4 Hz) | transient — busy, connecting, or an alarm |
| **Pulse** (60 ms flash) | a discrete event happened |
| **Code** (N fast flashes, 1.2 s pause, repeat) | a numbered fault, §5.3 |
| **Off** | absent / not applicable |

Colours keep their meaning across all modes: **blue = activity**,
**green = good**, **yellow = caution / secondary**, **red = fault**.

---

## 4. Hardware

### 4.1 Pin map

The panel takes over GPIO25/26 rather than adding to them. This was originally
argued on the grounds that their present meanings (`config parsed`,
`WiFi associated`) are preserved by the new encoding — but the simpler truth
(C9) is that **no LED has ever been fitted to GPIO2, 25 or 26 and nobody has
ever read one**. They are unused outputs, not an installed base, so this is a
reassignment rather than a migration and there is nothing to keep compatible.

| Signal | GPIO | Notes |
|---|---|---|
| RED | 32 | output only in this design; ADC1 unused here |
| YELLOW | 33 | " |
| GREEN | 25 | **was** `config loaded` |
| BLUE | 26 | **was** `WiFi connected` |
| Button | 13 | `INPUT_PULLUP`, other side to GND. Sits directly beside a GND pin on the 30‑pin DevKitC header |
| onboard LED | 2 | **unchanged** — keeps Story 2.4 / FR‑5 semantics verbatim, so the requirement is satisfied with the panel absent, disabled, or asleep |

Keeping GPIO2 unchanged is now a convenience rather than a compatibility
obligation: it costs nothing, it is the one indicator that needs no soldering,
and it is genuinely useful on the bench. But given C9, the honest reading is
that **FR‑5's "the Station signals a lost connection" is served for the first
time by this panel's red lane** — the onboard LED has been dutifully blinking
that signal at nobody since Story 2.4, and inside a sealed enclosure it will
continue to. Treat GPIO2 as a bench aid, and the red lane as the requirement.

`32, 33, 25, 26` are four consecutive header pins, immediately above the
existing sensor pins `27` (anemometer) and `34` (vane) on the EN/RST side —
one tidy run, one common ground rail. Verified unclaimed: `main.cpp` uses
2/25/26/27/34, `Wire` uses 21/22, nothing else in `src/` touches a GPIO.

Deliberately avoided: **GPIO12** (must read low at boot — an LED is *probably*
harmless there, but "probably" is not a reason to use a strapping pin);
**GPIO0/2/5/15** (strapping); **GPIO6–11** (flash); **GPIO34–39** (input only).
GPIO13 is not a strapping pin and has no boot‑time output glitch (GPIO14 does —
it would flash an LED at every reset, which is why the button is not there).

Pin assignments are compile‑time constants, overridable without editing source
via `build_flags = -DGUSTIK_PANEL_PIN_RED=32` etc.

### 4.2 Current‑limiting resistors — settled: 330 Ω on all four

Active high: `GPIO → R → LED anode`, `cathode → GND`, so
`digitalWrite(pin, HIGH)` lights it.

**All four lanes use 330 Ω.** It is the only value on hand, and Mlok confirmed
on 2026‑08‑16 that all four colours light visibly with it. The LEDs' own note
gives Vf 2.0–2.4 V, and the blue was measured at 2.6 V:

| Colour | Vf | I at 330 Ω from 3.3 V |
|---|---|---|
| Red / yellow / green | 2.0–2.4 V (per the kit's note) | 2.7–3.9 mA |
| Blue | 2.6 V (measured) | 2.1 mA |

`I = (3.3 V − Vf) / 330 Ω`. Every lane sits far inside the ESP32's 12 mA
recommended per‑pin figure (40 mA absolute maximum), and ~13 mA total with all
four solid — which is also why the power budget in §4.6 is as small as it is.
One value everywhere means no chance of fitting the wrong resistor to the wrong
lane, which is worth more here than an optimal per‑colour match.

**The high‑Vf green trap does not apply to this kit** (it was the one open
hardware question in the first draft, now closed). Bright‑green InGaN LEDs are
electrically blue dice and run at Vf ≈ 2.9–3.3 V, leaving almost nothing across
the resistor from a 3.3 V rail; these are 2.0–2.4 V parts and behave normally.
Keep it in mind only if a lane is ever replaced from a different batch — the
symptom is one colour visibly dimmer than the rest at the same resistor value,
and the check is the same one used on the blue here (LED in series with 1 kΩ
across 3.3 V, measure across the LED itself).

**Still unverified: daylight.** 2–4 mA is below the 4–6 mA these lanes were
originally sized for, and "visible on a bench" is not "visible on the water at
midday" — see §4.5. Check it outdoors before the enclosure is closed up. There
is headroom if a lane needs more punch: 220 Ω gives 4–6 mA and 150 Ω gives
6–9 mA, both still well inside the per‑pin limit. Prefer shading the panel over
raising the current — it is the cheaper fix and it costs no battery.

### 4.3 Wiring

```
                         ESP32 DevKitC (30-pin), EN/RST side
                        ┌───────────────────────────────────┐
                        │ ...                               │
             (vane)  34 ┤                                   │
                     35 ┤                                   │
                     32 ├──[330Ω]──▶|──┐   RED              │
                     33 ├──[330Ω]──▶|──┤   YELLOW           │
                     25 ├──[330Ω]──▶|──┤   GREEN            │
                     26 ├──[330Ω]──▶|──┤   BLUE             │
       (anemometer)  27 ┤              │                    │
                     14 ┤              │                    │
                     12 ┤              │  common cathode    │
                    GND ├──────────────┘  rail              │
                     13 ├──────┐                            │
                    ...        │                            │
                        └──────┼────────────────────────────┘
                               │
                          ┌────┴────┐
                          │  push   │   momentary, normally open
                          │ button  │   GPIO13 → button → GND
                          └────┬────┘   internal pull-up, active LOW
                               │
                              GND      (optional 100 nF across the button
                                        if the wiring run is long)

           ▶|  = LED, arrow points from anode (GPIO side) to cathode (GND side)
```

**Fitting fewer than four LEDs is supported.** In priority order: **RED**
(faults — the only one that tells you something is wrong), then **BLUE**
(alive/heartbeat — the bug‑030 detector), then **GREEN** (data is landing),
then **YELLOW**. With two LEDs fitted you still get every fault code and proof
of life; the detail modes just have fewer bar segments and degrade to
"only the fitted lanes are visible", no code change.

### 4.4 Button alternatives

- **No button fitted.** `INPUT_PULLUP` reads HIGH forever → no events → the
  panel stays in the default mode. Fully supported, and the mode this design
  expects for a first build.
- **The onboard `BOOT` button (GPIO0)** costs nothing to add as a second input
  and is handy on the bench. Two caveats: it is inaccessible inside a sealed
  enclosure, and holding it *during reset* drops the chip into the bootloader.
  Suggest supporting it only under `-DGUSTIK_PANEL_BOOT_BUTTON=1` for bench
  use, not as the boat control.
- **Sealed enclosure without drilling:** a **reed switch** to GND on GPIO13
  plus a small magnet held against a marked spot on the lid is electrically
  identical to a push button and needs no penetration at all. It also survives
  being under water, which a membrane switch does not. **Not a v1 concern** —
  with a cardboard box (C8) a plain push button poked through the side is
  fine, and the operator can reach in anyway. Park this with Story 5.2.

### 4.5 Visibility and the enclosure

**For v1 (cardboard box, C8) this section is almost entirely moot** — and that
is good news for the one thing §4.2 left open. A box shades the LEDs by
construction, an operator opens it or peers in, and nothing needs to be
mounted, drilled or sealed. Concretely, for the first deployment:

- Leave the LEDs on the breadboard or a scrap of protoboard, pointing up.
- The daylight question from §4.2 largely goes away: 2–4 mA is fine in shade,
  and inside a box everything is shade. Still worth a glance outdoors, but it
  is no longer a decision that blocks anything.
- **Label the lanes on the box itself** — marker pen on the cardboard is
  entirely adequate and better than the card in §11 for v1, because it cannot
  be lost.
- Water is the obvious risk with cardboard on a boat, and it is not
  LED‑specific — out of scope here, but it is the thing that decides how soon
  Story 5.2 matters.

**For Story 5.2's real enclosure**, when it exists, the original guidance still
applies and should be read then, not now:

- Direct sunlight defeats a 5 mm indicator at 2–4 mA. Mount the panel
  **shaded** — inside the cabin, under a lid overhang, or behind a recessed
  window — not on a sun‑facing top surface.
- Prefer **diffused** LEDs over water‑clear ones: water‑clear types have a
  ±15° viewing angle and vanish unless you look straight down the axis.
- Penetration options, cheapest first: LEDs mounted **inside** behind a clear
  polycarbonate window; drilled 5 mm holes with the LED bodies bedded in clear
  epoxy or hot‑melt from the inside (splash‑proof, not immersion‑proof); or
  3 mm acrylic rod light pipes.
- Only then does the reed‑switch button of §4.4 start to earn its place.

### 4.6 Power budget — re‑done for 4 × AA

The supply changed (C5): a powerbank that auto‑shut‑down after ~2 h even in
"low output" mode has been replaced by **4 × AA, ~3000 mAh nominal**, feeding
the DevKitC's `VIN`/5V pin through its onboard AMS1117 linear regulator. That
is a **3.3× smaller pack** than the powerbank this budget originally assumed,
so the panel's share is worth counting now.

| State | Draw | Over a 10 h race day | Share of 3000 mAh |
|---|---|---|---|
| ESP32 with Wi‑Fi (existing baseline) | ~80–160 mA avg | 800–1600 mAh | 27–53 % |
| Panel awake, typical (2 lanes solid) | +7 mA | +70 mAh | **2.3 %** |
| Panel awake, worst case (4 solid, 330 Ω) | +13 mA | +130 mAh | **4.3 %** |
| Panel asleep (60 ms pulse / 10 s) | +0.02 mA | +0.2 mAh | ~0 |
| Panel hard off | 0 | 0 | 0 |

**Conclusion: keep the 300 s sleep default.** With a powerbank the honest
answer was "battery is not the reason to auto‑sleep"; against 3000 mAh a panel
left awake all day costs 2–4 % of the pack, which is small but no longer noise.
Sleeping makes it exactly zero while preserving the "dark = dead" rule, so
there is now no argument on either side for leaving it awake.

**Three things about this supply that are not LED concerns but affect whether
the station survives the day** — recorded here because the numbers above are
meaningless if the pack cannot deliver them, and none of it is currently
written down anywhere:

- **A linear regulator throws away 45 % of a 6 V pack.** Runtime is
  `usable mAh / average mA` regardless of pack voltage, so the loss shows up as
  heat, not lost hours — but a small buck converter (~€2, into `VIN` at 5 V, or
  straight to the `3V3` pin bypassing the AMS1117) would roughly double the
  runtime for the same batteries. Much larger than anything in the table above.
- **Alkaline, not NiMH, for this wiring.** 4 × NiMH is 4.8 V nominal and sags
  to ~4.4 V under load, right at the AMS1117's dropout — the 3.3 V rail would
  brown out with most of the charge still in the cells. 4 × alkaline starts at
  ~6.4 V and stays above dropout until roughly 1.1 V/cell.
- **Expect 3000 mAh nominal to deliver noticeably less.** That figure is rated
  at a low discharge current; at ~120 mA continuous, alkaline AA cells give
  meaningfully less, and the regulator cuts out before the cells are empty.
  Plan on measuring a real run rather than trusting the label — that is Story
  5.1's endurance test, whose premise the powerbank finding just invalidated.

### 4.7 Battery sense (Q16) — accepted, built as phase 6

**Decided 2026‑08‑16: build it, as an independent phase after the panel
works.** The supply change opened a gap that did not exist before: **4 × AA has
no gauge**. A powerbank at least had its own charge LEDs; dry cells give no
warning at all, and the first symptom of a flat pack is the station going
silent mid‑regatta. The panel is already the thing an operator looks at, so
this is where the answer belongs.

It stays behind its own build flag and its own rollout phase because it is the
only part of this design that needs hardware beyond LEDs and a button —
bundling it would hold up the part that is already designed.

#### Divider

A 2‑resistor divider from `VIN` to GND, tap to **GPIO35** — input‑only, ADC1,
free, and on the same header run as the existing sensor pins. ADC1 matters:
ADC2 is unusable while Wi‑Fi is on, and the vane already proves ADC1 works here
(GPIO34 is ADC1_CH6, GPIO35 is CH7 — different channels of the same
multiplexed ADC, no conflict).

Target **ratio ≈ 1/3**, which puts a fresh 6.6 V pack at ~2.2 V — inside ADC1's
11 dB range with margin, and well clear of the badly compressed bottom of the
ADC's range that the vane bring‑up ran into.

| Resistors (top leg / bottom leg) | Ratio | 6.6 V reads | 4.3 V reads | Divider draw |
|---|---|---|---|---|
| 2 × 100 kΩ in series / 100 kΩ | 0.333 | 2.20 V | 1.43 V | 22 µA |
| 100 kΩ / 47 kΩ | 0.320 | 2.11 V | 1.37 V | 45 µA |
| 2 × 10 kΩ in series / 10 kΩ | 0.333 | 2.20 V | 1.43 V | 220 µA |

**The three‑equal‑resistors trick is the practical one** given that the only
value confirmed on hand is 330 Ω (§4.2): any single resistor value gives ratio
1/3 with two in series on top and one on the bottom, so whatever the kit
actually contains will work. Prefer 100 kΩ; 10 kΩ is fine too (220 µA over 10 h
is 2.2 mAh, 0.07 % of the pack). **Do not use two equal resistors** — ratio 1/2
puts 6.6 V at 3.3 V, past the ADC's range and past the pin's rating.

> **Measure the ratio, do not assume it.** Kit resistors are ±5 %, and that
> error scales the reading directly. Calibrate once with a multimeter across
> the pack, compare against what the firmware reports, and store the result in
> `config.txt` (below) — exactly the pattern `mag.offsetX`/`mag.offsetY` uses,
> for the same reason: a recalibration is then a `uploadfs`, not a reflash.

#### Firmware

Behind `-DGUSTIK_PANEL_BATTERY=1`; with the flag absent, nothing below is
compiled, GPIO35 is untouched, and the mode cycle is 1→2→3→4→1 as before.

- Sample once per cycle alongside the vane, via `analogReadMilliVolts()` —
  eFuse‑calibrated, unlike raw `analogRead()` (established during the vane
  work, cerebrum).
- **Smooth before deciding.** Wi‑Fi TX bursts sag the pack by a few hundred
  millivolts; a single sample landing on a burst reads low and would
  false‑alarm. Use a slow EMA across cycles, and require a threshold to stay
  crossed for **3 consecutive cycles** before changing state. Hysteresis on the
  way back up, so a recovering pack does not oscillate between warn and ok.
- **Implausible reading ⇒ `unknown` ⇒ no battery indication at all.** A
  computed pack voltage below 3.0 V or above 8.0 V means the divider is not
  fitted, is miswired, or the pin is floating. This is §7.3's "absent hardware
  is a no‑op" invariant applied here, and it is what keeps a half‑built board
  from flashing a permanent low‑battery alarm.

#### Thresholds

Pack voltage, not cell voltage, measured under load:

| State | Pack | Per cell | Meaning |
|---|---|---|---|
| ok | ≥ 5.0 V | ≥ 1.25 V | fine |
| **warn** | 4.6 – 5.0 V | 1.15 – 1.25 V | the alkaline knee is near — have spares to hand |
| **critical** | < 4.6 V | < 1.15 V | AMS1117 dropout is 3.3 V + ~1.1 V ≈ 4.4 V; brown‑out is minutes away |
| unknown | < 3.0 V or > 8.0 V | — | divider absent or miswired; indicate nothing |

> Voltage under load is a **rough** proxy for remaining charge on alkalines,
> and it recovers when the load drops. Treat the bar as "roughly how worried to
> be", never as a fuel gauge — and say so in the manual.

#### Indication

Deliberately *not* a ninth fault code — the eight‑code cap in §5.3 exists
because counting past eight on a moving boat does not work. Instead:

- **A global override** (§5.10): all four lanes fast‑blink in unison for 2 s,
  in any mode including sleep. **Every 120 s at `warn`, every 30 s at
  `critical`** — same unmistakable signal, two rates for two urgencies, no
  counting and no new vocabulary.
- **Mode 5 — Battery** (§5.9) for the detail, present in the mode cycle only
  when the flag is on.

#### Config keys

```ini
# Battery sense (phase 6). All optional; omit for the compiled-in defaults.
battery.dividerRatio=0.3333    # MEASURED, not nominal - see above
battery.warnVolts=5.0
battery.criticalVolts=4.6
```

#### Later, separately: ship the number to the backend

Pack voltage graphed on `/status.html` beside RSSI would answer "how long does
a set of cells actually last" properly, and turn Story 5.1's endurance test
into a chart rather than a stopwatch. That touches the firmware payload, the
ingest route, the DB schema and the status page, so it is **phase 7**, not part
of phase 6. Noted here so it is not forgotten.

---

## 5. Encoding

### 5.1 Boot self‑test

At `setup()`: all four LEDs on for 400 ms, then a 100 ms sweep RED→YELLOW→
GREEN→BLUE, then the panel enters mode 1. Total 800 ms, no blocking (it runs
as a normal panel state while `setup()`'s existing work proceeds — see §8).

This makes a dead LED, a cold solder joint, or a wrong resistor obvious at
power‑on, and gives the person a positive "it just started" signal.

It also earns a second job for free under the new supply (C5): **a self‑test
that keeps repeating means the station is rebooting**, and on 4 × AA the
overwhelmingly likely cause is the pack sagging below the regulator's dropout
and tripping the brown‑out detector. Flat batteries therefore have a visible
symptom even without the optional battery sense of §4.7 — the panel restarting
over and over, which is hard to mistake for anything else.

### 5.2 Mode 1 — Status (default)

| Lane | Solid | Slow blink | Fast blink | Off |
|---|---|---|---|---|
| 🔵 **BLUE** — life | — | — | — | **loop hung or no power** |
| | one pulse per sample cycle (3 s) = loop alive · **double** pulse = flash buffer non‑empty (Q10) | | | |
| 🟢 **GREEN** — data landing | last POST stored ≥ 1 row | 2xx but stored **nothing** (Q9) or a backend too old to report counts | — | last send failed |
| 🟡 **YELLOW** — radio | associated, RSSI ≥ −67 dBm | associated but weak, RSSI < −75 dBm | scanning / connecting | not associated (Q4) |
| 🔴 **RED** — fault | fatal, station cannot work at all | *(not used)* | — | no fault |
| | otherwise blinks the **fault code**, §5.3 | | | |

The two properties worth memorising:

- **All four dark ⇒ not running.** No power, or a crash before `setup()`
  finished. (This rule is what the sleep behaviour in §5.7 is designed to
  preserve, and what "hard off" deliberately breaks — see the warning there.)
- **Blue dark, anything else lit ⇒ the loop is hung.** The bug‑030 signature,
  which previously produced no symptom at all.

### 5.3 Fault codes (red lane)

N fast flashes (150 ms on / 150 ms off), then a 1.2 s pause, repeating. Only
the **highest‑priority active** fault is shown; `/status.html` has the full
picture when a phone is available.

| Code | Meaning | Question | What to do on the boat |
|---:|---|---|---|
| — (off) | no fault | | |
| **solid** | fatal — `setup()` could not complete | | power‑cycle; then reflash |
| **1** | no config: `config.txt` missing, unparseable, or zero networks | Q3 | re‑run `uploadfs` |
| **2** | no configured network in range, or association keeps failing | Q4 | check the AP / hotspot is on, move closer |
| **3** | Wi‑Fi up, backend unreachable (no HTTP status: DNS, route, TLS, timeout) | Q7 | the AP has no route out — check the hotspot's data |
| **4** | backend rejected us: 401 or other 4xx | Q8 | wrong `backend.token`, re‑upload config |
| **5** | 2xx but **nothing stored** (`inserted == 0`) | Q9 | the bug‑031 signature — reboot the Station |
| **6** | buffering: last send failed, readings queuing to flash | Q10 | usually transient; a persistent 6 means 2/3/4 is intermittent |
| **7** | clock never synced (NTP failed) | Q11 | timestamps unreliable; needs real internet, not just an AP |
| **8** | a sensor is failing | Q12–Q14 | press the button to mode 3 to see which |

Eight codes is the cap on purpose: counting past eight flashes on a moving
boat does not work. Anything finer lives in mode 3 or on the status page.

### 5.4 Mode 2 — Signal (Q5, Q6)

Bar graph of current RSSI, lit **from the red end**: red → +yellow → +green →
+blue. Thresholds are the same ones `/status.html` already draws reference
lines at, so the LED and the web view never disagree.

| Lit | RSSI |
|---:|---|
| 4 | ≥ −55 dBm — excellent |
| 3 | −55 … −67 dBm — good (`/status.html`'s −67 line) |
| 2 | −67 … −80 dBm — usable (its −80 line) |
| 1 | −80 … −90 dBm — marginal, expect dropouts |
| 0, red fast‑blinking | < −90 dBm or not associated |

**Which network you are on** (Q5) rides on the same four LEDs with no counting:
a **steady** bar means network 1 (`network1.*`, the shore/primary Wi‑Fi); a bar
that **pulses off briefly once a second** means network 2 — "pulsing means
you're on somebody's phone".

This is the mode to hold the panel in while anchoring, or while walking an AP
around the shore.

### 5.5 Mode 3 — Sensors (Q12, Q13, Q14)

| Lane | Meaning |
|---|---|
| 🔵 BLUE | **one 30 ms pulse per anemometer reed closure, live.** Dark while the cups are turning ⇒ the anemometer is not wired (bug‑059, exactly). This doubles as the wind‑speed‑as‑blink‑rate readout: ~1 pulse per revolution, so ~4 Hz at 5 m/s |
| 🟢 GREEN | vane: solid = ADC inside a known detent band · slow blink = outside every band (open or shorted wiring) |
| 🟡 YELLOW | magnetometer: solid = last I2C read succeeded · slow blink = failing, headings are stale |
| 🔴 RED | off if all three are fine, slow blink otherwise |

This is the mode that would have turned bug‑059 from a multi‑hour investigation
into "spin the cups, watch the blue LED".

### 5.6 Mode 4 — Wind (Q15)

Bar graph of the current wind speed, lit from the red end, on the boundaries
`backend/src/static/beaufort.js` already uses — so the LEDs, the dashboard and
the Beaufort label all agree:

| Lit | Speed | Beaufort |
|---:|---|---|
| 0 | < 1.6 m/s | 0–1 — bezvětří / vánek |
| 1 | 1.6 – 3.4 | 2 — slabý vítr |
| 2 | 3.4 – 5.5 | 3 — mírný vítr |
| 3 | 5.5 – 8.0 | 4 — dosti čerstvý vítr |
| 4 | ≥ 8.0 | 5+ — čerstvý vítr and above |
| 4, all fast‑blinking | ≥ 10.8 | 6+ — silný vítr, the capsize‑risk cue for P550s |

**Direction**, once on entry and then every 5 s: the bar goes dark and the
yellow lane blinks `octant + 1` times — 1 = S, 2 = SV, 3 = V, 4 = JV, 5 = J,
6 = JZ, 7 = Z, 8 = SZ (Czech, matching `compass.js`; octant indices as defined
by `correct/wind_direction.h`). This is the **first thing to cut** if flash is
tight — it is the least‑used item here, and it only matters when the network
is down, which is admittedly exactly when the panel matters most.

### 5.7 Sleep, and hard off

- After `leds.timeoutSeconds` (**default 300 s**) with no button press, the
  panel **sleeps**: everything dark except a blue proof‑of‑life pulse every
  10 s and, if a fault is active, its red code repeated every 10 s instead of
  every 2 s.
- Any button press wakes it to mode 1 and restarts the timeout.
- A non‑default mode also **auto‑returns to mode 1 after 60 s**, so the panel
  is never found parked in "wind bar".
- `leds.timeoutSeconds=0` disables sleeping.
- **Long press (≥ 2 s) = hard off**, everything dark including the heartbeat.
  Another long press restores it.

> **The one trap in this design:** hard off breaks "all dark ⇒ not running".
> Mitigation: hard off is **runtime only and never persisted** — any reboot or
> power cycle comes back with the panel on and running its self‑test. The
> durable way to silence the panel is `leds.enabled=false` in `config.txt`, and
> that is the setting the manual tells people to use if they want it quiet for
> good.

### 5.8 Mode banner

On entering mode N, all four LEDs flash together N times (100 ms on / 100 ms
off) before the mode's own display starts. Without it, "which mode am I in?" is
unanswerable, and it reuses the self‑test primitive so it is nearly free.

### 5.9 Mode 5 — Battery (phase 6, `-DGUSTIK_PANEL_BATTERY=1` only)

Bar graph of pack voltage, lit from the red end, on the §4.7 thresholds. In the
mode cycle only when the flag is on — without it the cycle is 1→2→3→4→1 and
nothing here exists.

| Lit | Pack | Per cell |
|---:|---|---|
| 4 | ≥ 5.6 V | ≥ 1.40 V |
| 3 | 5.2 – 5.6 V | 1.30 – 1.40 V |
| 2 | 5.0 – 5.2 V | 1.25 – 1.30 V |
| 1 | 4.6 – 5.0 V | 1.15 – 1.25 V — `warn` |
| 0, red fast‑blinking | < 4.6 V | `critical` |
| all four **slow**‑blinking | — | `unknown`: divider not fitted or miswired (§4.7) |

`unknown` gets its own pattern rather than showing zero bars, because "no
divider" and "flat pack" must not look alike.

### 5.10 Low‑battery override (phase 6)

The one signal that ignores modes entirely. All four lanes fast‑blink in unison
for 2 s, then the panel returns to whatever it was doing:

| State | Repeat | Also fires while asleep? |
|---|---|---|
| `warn` | every 120 s | yes |
| `critical` | every 30 s | yes |
| `ok` / `unknown` | never | — |

It fires **through sleep** on purpose: a pack going flat overnight or during a
long postponement is exactly when nobody is pressing buttons. It does **not**
fire when the panel is hard off (§5.7) — hard off means off, and it is runtime
only, so a power cycle restores it.

This is the only pattern in the design that uses all four lanes at once outside
the boot self‑test and the mode banner, which is what makes it unmistakable.

---

## 6. Button behaviour

| Gesture | Effect |
|---|---|
| press < 30 ms | ignored (debounce) |
| **short press** (30 ms – 800 ms) | wake if asleep; otherwise advance mode 1→2→3→4→1 (→5→1 once phase 6's battery mode exists, §5.9) |
| **long press** (≥ 2 s) | toggle hard off (§5.7) — acts on release, so it never also counts as a short press |
| held at boot | nothing; GPIO13 is not a strapping pin, so this is safe |

Debounce is 30 ms of stable level. The decoder is a pure state machine fed
`(level, nowMs)` and returning `None | Short | Long`, so all of the above is
unit‑testable on the host with no hardware (§10).

---

## 7. Optionality — three independent layers

The feature is optional in three separate senses, and each one alone is
sufficient.

**7.1 Build time — the flash guarantee.** `-DGUSTIK_STATUS_PANEL=0` compiles
the panel out entirely: the `indicate/` sources are never referenced, and with
PlatformIO's default `-ffunction-sections -Wl,--gc-sections` the linker drops
them wholesale. Flash cost is then **exactly zero bytes**, and GPIO25/26 revert
to their present `config loaded` / `WiFi connected` behaviour. This is the
answer to C1: if the panel does not fit, it is one flag and the build is
byte‑identical to today's.

**7.2 Config time — no reflash.** New `config.txt` keys, both optional:

```ini
# Status LED panel. Omit both keys for the defaults (enabled, 5-minute sleep).
leds.enabled=true
leds.timeoutSeconds=300        # 0 = never sleep
```

`leds.enabled=false` leaves the pins low from boot and ignores the button.
Parsed by `parseStationConfig()` alongside `mag.offset*`, so changing it costs
`pio run -t uploadfs`, not a reflash — the same rationale that put the
magnetometer calibration there. Phase 6 adds `battery.dividerRatio`,
`battery.warnVolts` and `battery.criticalVolts` on the same terms (§4.7); the
divider ratio in particular *must* be a config key rather than a constant,
because it is a measured property of two specific resistors.

**7.3 Wiring time — absent hardware is a no‑op.** Driving a GPIO with nothing
attached is harmless, and an unwired `INPUT_PULLUP` button reads "released"
forever. A Station with the panel compiled in and *nothing* soldered behaves
exactly as it does today. This must hold as a **tested invariant**, not an
assumption: the panel is the only consumer of its own state.

---

## 8. Firmware architecture

Follows the project's existing pure‑vs‑hardware split, so almost all of it is
testable in `[env:native]`.

```
firmware/src/indicate/
  panel_inputs.h      pure   PanelInputs: the snapshot the panel reads (§8.1)
  pattern.h/.cpp      pure   LanePattern enum + isLit(pattern, nowMs, phase0)
  fault.h/.cpp        pure   PanelInputs -> highest-priority fault code (0-8)
  panel.h/.cpp        pure   the mode machine: mode, sleep timer, banner,
                             bar mappings -> 4 LanePatterns
  button.h/.cpp       pure   debounce + short/long decoder ((level,ms)->event)
  battery.h/.cpp      pure   phase 6: mV -> pack volts -> EMA -> state machine
                             (ok/warn/critical/unknown) + bar segments
  hw/led_panel.cpp    hw     pinMode/digitalWrite on the 4 pins
  hw/button_pin.cpp   hw     digitalRead of GPIO13
  hw/battery_adc.cpp  hw     phase 6: analogReadMilliVolts(GPIO35)
```

`indicate/` matches the verb naming of `sense/`, `correct/`, `transmit/`.

### 8.1 Data flow — one way, always

```
  sense/ ─┐
transmit/ ├─▶ PanelInputs (plain struct, copied once per loop iteration)
  config/ ┘            │
                       ▼
              StatusPanel::tick(inputs, nowMs, buttonEvent)
                       │
                       ▼
              PanelOutputs { LanePattern red, yellow, green, blue }
                       │
                       ▼
              hw/led_panel: 4 × digitalWrite
```

**Invariant (C3): nothing downstream of `PanelInputs` may write to anything
upstream of it.** The panel cannot delay a sample, cannot clear a buffer,
cannot touch Wi‑Fi. This is what makes "the panel is optional" true rather
than hopeful, and it is the thing to check in review.

`PanelInputs` is roughly: `{ loopAliveMs, wifiAssociated, rssiDbm, rssiValid,
networkIndex, lastSendOk, lastHttpStatus, lastInserted, hasCounts,
bufferedCount, clockSynced, configLoaded, magnetometerOk, vaneInRange,
pulseCountSnapshot, windSpeedMs, windDirOctant }` — ~40 bytes, all already
computed in `loop()` today.

### 8.2 The one change to `main.cpp`'s shape

`loop()` currently early‑returns until the 3 s sample interval elapses, which
makes any sub‑second blinking impossible. The panel tick has to run *before*
that gate:

```cpp
void loop() {
    unsigned long now = millis();
    panelTick(now);                                   // new: cheap, non-blocking
    if (now - lastSampleAt < kSampleIntervalMs) {
        return;
    }
    /* ... existing sampling / transmit block, unchanged ... */
    panelInputs = snapshotPanelInputs(/* the values it already computes */);
}
```

Note `loop()` today busy‑spins at 100 % CPU between samples. That is
pre‑existing and out of scope here, but it is worth recording that a `delay(2)`
in the tick path would cut CPU (and therefore current draw) substantially while
still giving the panel ~500 Hz — likely a bigger battery win than the entire
panel costs. **Not** part of this change; flagged for a separate look, since
changing loop timing deserves its own verification.

### 8.3 One new sensor API

`Anemometer::pulseCountSnapshot()` — a non‑resetting read of the volatile
counter, for the live blue pulses in mode 3. 32‑bit aligned reads are atomic on
this core, so it needs no `noInterrupts()`. The panel only compares successive
snapshots and flashes on an increase; a *decrease* (which happens each time the
sample cycle calls `readAndResetPulseCount()`) is treated as a resync, not an
event. The existing read‑and‑reset path is untouched, so measurement is
unaffected — this is exactly what invariant §8.1 requires.

### 8.4 What the panel must never do

No `delay()`, no dynamic allocation, no `String`, no ISR, no `Serial.printf`
inside the tick (one short line on a mode change at most — new format strings
cost flash, §9), no floating‑point beyond the two bar comparisons, no writes to
LittleFS.

---

## 9. Flash and RAM budget

**Measured today, before any change** (`pio run -e esp32dev`):

```
RAM:    15.8 %  (51 732 / 327 680 B)
Flash:  92.4 %  (1 210 461 / 1 310 720 B)   →  100 259 B free
```

**Estimated cost of this design:** 2–4 KB flash (a state machine plus small
constant tables, no new libraries, no new format strings) and under 200 B of
RAM. Phase 6's battery sense adds perhaps another 0.5–1 KB — one ADC read, an
EMA, and a four‑state comparison — and is separately switchable. That fits in the current headroom roughly twenty times over — the risk is
not this feature, it is that the project is already at 92.4 % with no margin
for the *next* one.

**Mitigation ladder, cheapest first:**

1. **Enlarge the app partition — the real fix, and it is free.** The default
   4 MB layout reserves a whole second 1.25 MB OTA slot that this project can
   never use: there is no OTA mechanism, and every update is a USB reflash. A
   custom `firmware/partitions_gustik.csv`:

   ```csv
   # Name,   Type, SubType, Offset,   Size,     Flags
   nvs,      data, nvs,     0x9000,   0x5000,
   otadata,  data, ota,     0xe000,   0x2000,
   app0,     app,  ota_0,   0x10000,  0x200000,
   spiffs,   data, spiffs,  0x210000, 0x1E0000,
   coredump, data, coredump,0x3F0000, 0x10000,
   ```

   with `board_build.partitions = partitions_gustik.csv`, takes the app
   partition from 1.25 MB to **2 MB** (92.4 % → **57.7 %**) *and* grows the
   filesystem from 1.375 MB to **1.875 MB**. Both numbers improve; nothing is
   given up.

   Two details that matter: the filesystem partition **must keep the label
   `spiffs`**, because that is the default label `LittleFS.begin()` looks for
   in arduino‑esp32 — renaming it to `littlefs` silently breaks config
   loading. And changing the layout invalidates the existing filesystem
   contents, so this is a `pio run -t upload` **and** `-t uploadfs` together,
   with `config.txt` re‑uploaded.

   (`huge_app.csv` is the off‑the‑shelf alternative — 3 MB app — but it shrinks
   the filesystem to 896 KB, which is the wrong trade here, see the note below.)

2. **`-DGUSTIK_PANEL_STATIC=1`** — drop the pattern engine, lanes become plain
   on/off. Loses fault codes, bars and the heartbeat; keeps mode 1's four
   states. Saves perhaps 1–1.5 KB. This is the "eliminate blinking" fallback.
3. **Mode 1 only** — drop modes 2–4 and the button entirely.
4. **`-DGUSTIK_STATUS_PANEL=0`** — zero bytes, §7.1.

Whichever is chosen, **measure and record the before/after flash figure** in
the commit message, as this project already does.

> **Separate finding, not part of this feature, flagged because §9.1 touches
> the same partition.** `FlashBuffer` writes **one LittleFS file per reading**
> and `computeBufferCapacityForHours(4, 3)` asks for **4800** of them. LittleFS
> allocates in 4 KB blocks; unless every record is small enough to be stored
> inline in directory metadata, 4800 files cannot fit in the current 1.375 MB
> filesystem — the 4 h buffer target (NFR‑4) may be quietly unmet on real
> hardware. This is a suspicion from reading the code, **not a measurement**;
> it wants a real test (fill the buffer during a forced outage and count what
> comes back). Noted here because shrinking the filesystem — which
> `huge_app.csv` would do — could make an existing problem worse, and because
> the fix (one append‑only file, or fixed‑size records) is unrelated to LEDs.

---

## 10. Testing

**Native (`pio test -e native`), ~25 new tests, no hardware:**

| Suite | Covers |
|---|---|
| `test_panel_pattern` | each pattern's duty cycle and phase; code‑N produces exactly N flashes then a pause |
| `test_panel_fault` | priority ordering; every fault in §5.3 reachable; no fault ⇒ code 0; the bug‑031 input (2xx, `inserted == 0`) ⇒ code 5; `hasCounts == false` never fabricates code 5 |
| `test_panel_modes` | short‑press cycling, banner, 60 s auto‑return, sleep timeout, hard‑off toggle, `leds.enabled=false` ⇒ all lanes always off |
| `test_panel_bars` | RSSI banding at every boundary incl. exactly −67/−80; wind banding against `beaufort.js`'s boundaries; below‑minimum and absurd inputs are total |
| `test_button` | debounce, short vs long, a long press never also emits a short, no event when the pin is stuck HIGH (the "no button fitted" invariant) |
| `test_station_config` (extend) | `leds.*` parsing, defaults when absent, malformed values ⇒ defaults not garbage; phase 6 adds `battery.*` |
| `test_battery` *(phase 6)* | threshold boundaries and hysteresis; EMA rejects a single TX‑sag sample; 3‑cycle confirmation before a state change; implausible readings ⇒ `unknown` and **no** indication (the "divider not fitted" invariant); bar segments at every boundary; `unknown` never renders as zero bars |

**On hardware:** a `[env:panel_diag]` bring‑up sketch that walks the four LEDs
and prints one raw line per button transition (`BTN <micros> <level>`) — dumb
firmware, eyes as the analysis, per the standing bring‑up preference. It exists
to prove the *wiring*, not the logic. Same `build_src_filter = -<*> +<...>`
isolation as `mag_diag` / `pulse_diag`.

**Manual checks on the real Station:** boot self‑test visible; pull the AP →
red code 2 within one cycle; wrong token → code 4; spin the cups → blue pulses
in mode 3; unplug the magnetometer → yellow blink in mode 3 (and code 8 in
mode 1); leave it 5 minutes → sleeps to a heartbeat.

---

## 11. Documentation deliverables

1. **`docs/hardware/status-led-panel.md`** — the wiring, resistor table and Vf
   measurement method from §4, in the same shape as the existing
   `wind-sensor-wiring.md` / `magnetometer-wiring.md`.
2. **`manual.html` — a Czech section**, since that is what the person on the
   boat reads and the whole UI is Czech. Needs the lane table, the fault‑code
   table, and the button gestures. Note the Czech‑abbreviation rule from
   `compass.js` applies to the direction blink code (`S` = *sever*, north —
   always spell the word out).
3. **The legend written where the LEDs are** — the fault codes are useless if
   they only exist on a web page you cannot reach when code 3 is flashing. For
   v1 that means **marker pen on the cardboard box** (C8), which cannot be lost
   and costs nothing; the printed card below is for Story 5.2's real enclosure.
   Either way the content is the same A7 block:

   ```
   ┌─────────────────────────────────────────┐
   │  GUSTÍK — stavové diody                 │
   │  🔵 bliká = běží   🟢 svítí = data jdou │
   │  🟡 svítí = wifi   🔴 bliká = chyba č.: │
   │  1 chybí config    5 backend neuložil   │
   │  2 není wifi       6 ukládám do paměti  │
   │  3 backend neodpovídá  7 nesynchro. čas │
   │  4 odmítnutý token 8 vadné čidlo (→3×)  │
   │  všechny 4 blikají naráz = SLABÉ BATERIE│
   │  tlačítko: krátce = režim, dlouze = zhas│
   └─────────────────────────────────────────┘
   ```

   The battery line is added by phase 6 (§5.10). It is worth the space even
   though it is the last thing built: it is the one signal that fires without
   anyone asking for it, so it is the one most likely to be seen by someone who
   has never read the legend.

---

## 12. Open questions for Mlok

1. ~~**Green LED Vf**~~ and ~~**daylight visibility**~~ — **both closed
   2026‑08‑16.** All four colours are 2.0–2.4 V parts lighting visibly on the
   available 330 Ω, so one resistor value throughout (§4.2); and with no
   enclosure, the box shades the panel anyway (§4.5). Neither blocks anything.
2. **Button: fit one or not?** Everything in mode 1 and the fault codes works
   without it. Modes 2–4 are the payoff; mode 2 (signal while anchoring) is
   probably the one that justifies it.
3. ~~**Button style**~~ — **deferred, not open.** With a cardboard box a plain
   push button is fine; the reed‑switch‑and‑magnet option (§4.4) is a Story 5.2
   decision and should be made with the enclosure, not now.
4. **Partition table** — adopt `partitions_gustik.csv` (§9.1) now, as a
   separate commit before the panel? It is a strict improvement but it forces a
   `uploadfs` and a full reflash, so it should not be bundled into a feature
   commit.
5. **Default `leds.timeoutSeconds`** — 300 s proposed. Longer for a first
   season while trust is being built?
6. **Direction blink code in mode 4** — keep or cut (§5.6)?
7. ~~**Battery sense — build it, or not?**~~ — **decided 2026‑08‑16: yes, as a
   later phase.** Fully specified in §4.7, §5.9, §5.10; rollout phase 6. One
   sub‑question deliberately left to build time: which resistor values are
   actually in the kit (§4.7's table gives three options, all ratio ≈ 1/3).
8. **Not this design, but adjacent and probably worth more than all of it:** a
   ~€2 buck converter would roughly double runtime on the same cells (§4.6).
   Worth a look before Story 5.1's endurance test, since it changes what that
   test is measuring.

---

## 13. Rollout

| Phase | Content | Verifiable by |
|---|---|---|
| 0 | *(optional, separate commit)* partition table §9.1 | flash % drops to ~58 %, config still loads after `uploadfs` |
| 1 | `indicate/` pure modules + tests; `-DGUSTIK_STATUS_PANEL=0` default | `pio test -e native`; `pio run -e esp32dev` byte‑identical flash figure |
| 2 | hw glue, `main.cpp` wiring, `leds.*` config keys, flag on | flash delta recorded; Station behaves identically with nothing soldered |
| 3 | 4 LEDs (330 Ω each) + button on breadboard/protoboard; `panel_diag`; manual checks §10 | by eye on the bench — no enclosure work needed for v1 (C8) |
| 4 | `manual.html` Czech section, `docs/hardware/` page, legend on the box | reviewed on a phone |
| 5 | one regatta day of real use | does anyone actually look at it? |
| 6 | **battery sense** §4.7 / §5.9 / §5.10 — divider, `battery.*` config keys, mode 5, low‑battery override | reported voltage tracks a multimeter across the pack; alarm fires on a deliberately run‑down set; divider unplugged ⇒ `unknown`, no indication, no false alarm |
| 7 | *(separate, backend)* ship pack voltage with each reading, graph it on `/status.html` | Story 5.1's endurance test becomes a chart |

Phases 1 and 2 are safe to land with the flag off — nothing changes on the
running Station until phase 3 puts real LEDs on real pins.
