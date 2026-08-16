# Status LED panel + mode button — design

**Date:** 2026-08-16 · **Status:** design settled; **phases 0–2 implemented**
(partition table flashed; `indicate/` modules, hardware glue, `main.cpp`
wiring and `leds.*` config keys built and tested — see §13). Phase 3 is
physical: nothing is soldered yet. ·
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
| C1 | Flash: **57.7 % used, ~866 KB free** after the phase‑0 partition change (§9.1) | Not a binding constraint. The build flag and the table‑driven blink engine are kept because they are good design, not because flash forces them |
| C2 | `loop()` must never block or be delayed | Panel is a non‑blocking tick, no `delay()`, no ISR, no allocation |
| C3 | The panel must never affect measurement or transmission | One‑way data flow: the panel **reads** a snapshot, never writes to anything else (§8) |
| C4 | Hardware may be absent | Missing LEDs and a missing button must both be silently harmless (§7) |
| C5 | Supply is **4 × AA (~3000 mAh)**. Not a powerbank — those auto‑shut‑down after ~2 h even in "low output" mode (bug‑061) | Auto‑sleep has a battery justification as well as a distraction one; budget in §4.6 |
| C6 | LEDs are a distraction on a boat, especially at dusk | Auto‑sleep to a heartbeat; a long press kills them entirely |
| C7 | Kit LEDs (red/yellow/green/blue, **Vf 2.0–2.4 V** per their note; blue measured 2.6 V) and **330 Ω is the only resistor value on hand**. Up to **5 LEDs of each colour** are available; **9 resistors** is the agreed budget | One resistor value throughout at ~2–4 mA per LED. Nine LEDs in one flat row, arranged as two groups (§3, §4.1) |
| C10 | **No switch of any kind is available.** The battery pack is connected by plugging a wire into the breadboard | No power switch in the design. This dissolves the divider standby‑drain problem (§4.7) and adds a new brown‑out cause (§4.6, §5.1) |
| C11 | **The 5 V rail is sometimes fed from USB**, especially during development. Pack and USB are never connected simultaneously | Battery sense cannot distinguish USB from a half‑flat pack by voltage alone, and **no attempt is made to**. It measures the rail; on USB it will report low and raise the alarm. Accepted (§4.7) |
| C12 | The board and the magnetometer are permanently mounted on a **steel plate**, on one large breadboard | Magnetic, not electrical. The panel's own currents are irrelevant (§4.8); the calibration *ordering* it forces is not |
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
the encoding in §5 is just how it gets onto nine LEDs.

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
| Q16 | **How much battery is left?** | The station going dark mid‑regatta — **4 × AA has no gauge of any kind** | ❌ — answered by phase 6, §4.7 |

Two observations shape everything below:

- **Q2 is the highest‑value item and it is free.** A heartbeat LED makes
  "hung" distinguishable from "unpowered" at a glance — the one symptom
  bug‑030 conspicuously did not produce.
- **Q6, Q12 and Q15 are the only ones that are *actionable while standing
  there*.** They justify the button: they need more resolution than a
  binary lamp, but only occasionally.

---

## 3. Design shape

Nine LEDs in one flat row, split into **two groups that are different kinds of
display**:

```
  R   Y   G   B            G   G   Y   Y   R
 └───── status ─────┘     └────── detail ──────┘
   four indicators           a green→red ramp
   colour = identity         colour = severity
   always on                 button selects what it shows
```

The split is the whole design, and it rests on two rules that must not be
traded away:

- **Status is never hidden.** Nothing the button does can blank the fault lane,
  the heartbeat or the data lane. A panel that hides a fault while someone
  checks the wind is worse than no panel.
- **Colour carries exactly one meaning per group.** Within the status group
  colour is *identity* (yellow = radio); within the detail group it is
  *magnitude* (yellow = middling). Re‑using one group for both makes a colour
  unreadable until you know which mode you are in.

With nine, the left group never changes and the right group is unmistakably a
meter. A mixed‑colour cluster reads as a set of separate indicators; an ordered
green‑to‑red ramp reads as a scale. Both idioms are universal — instrument
panels and fuel gauges — so nobody has to be taught which group is which. A
physical gap and the legend (§11) reinforce it.

Note the two reds sit at the **extreme opposite ends of the row**, as far apart
as the layout allows, and **blue appears exactly once in the whole panel**.
Blue only ever means "the loop is alive".

So the panel is layered:

1. **The status group**, four independent lanes, each a fixed colour with a
   fixed meaning, readable without counting. Answers Q1–Q4, Q7, Q9, Q10
   immediately and points at the rest. **Never changes.**
2. **A fault code** on the status red lane (N flashes, pause, repeat) naming
   *which* problem, for when "something is wrong" is not enough. Q3, Q7, Q8,
   Q9, Q11 collapse into this.
3. **The detail group**, a five‑position scale showing one quantity at a time.
   Defaults to **wind** (Q15) — so with nobody touching anything the panel
   answers both "is it working" and "how windy is it". The button cycles it
   through signal (Q5, Q6), sensors (Q12–Q14) and, in phase 6, battery (Q16).
4. **Sleep**, so that none of this blinks at anybody for eight hours.

The button is only ever needed for layer 3, and only to move *off* the default.
Layers 1, 2 and 4 work with no button fitted at all.

### Reading rules (learn once, apply everywhere)

| Pattern | Means |
|---|---|
| **Solid** | good / present / confirmed |
| **Slow blink** (1 Hz) | attention — working, but degraded |
| **Fast blink** (4 Hz) | transient — busy, connecting, or an alarm |
| **Pulse** (60 ms flash) | a discrete event happened |
| **Code** (N fast flashes, 1.2 s pause, repeat) | a numbered fault, §5.3 |
| **Off** | absent / not applicable |

**In the status group, colour is identity** and holds across every mode:
blue = activity, green = good, yellow = caution/secondary, red = fault.

**In the detail group, colour is severity** and holds across every mode:
green = fine, yellow = paying attention, red = the extreme. Exactly **one LED
is lit at a time**, and it slides left→right as the situation deteriorates.

That last rule is why the detail group is a **dot and not a filled bar**. Three
reasons, each of which rules out a fill on its own:

- A dot does not look like a signal bar, so it never invokes the "more bars =
  better" expectation from phones. A fill does, and that forces an inverted
  reading for signal and battery while wind wants the opposite direction — two
  conventions fighting in one display.
- A fill lit from the red end leaves red on at every level ≥ 1. On a
  permanently‑visible group that fights the status group's red = fault
  directly. With a dot, red is dark until it means something.
- You read the value **by colour alone**, with no counting and no judging bar
  length on a rocking boat — the same reasoning that caps the fault codes at
  eight.

One accessibility consequence worth having for free: meaning is carried by
**position as well as colour**, so the detail group stays readable with a
red‑green deficiency, which a pure red/green indicator panel would not.

---

## 4. Hardware

### 4.1 Pin map

The panel takes over GPIO25/26 rather than adding to them. **No LED has ever
been fitted to GPIO2, 25 or 26 and nobody has ever read one** (C9), so these are
unused outputs, not an installed base — a reassignment rather than a migration,
with nothing to keep compatible.

**The board is not a DevKitC.** Probed over serial (`esptool.py flash_id`): **ESP32‑D0WD‑V3 rev 3.1** (the die in a WROOM‑32E
module), **4 MB flash**, 40 MHz crystal, CP2102 bridge, MAC `30:76:f5:b9:13:04`.
Together with the silkscreen (`V5`, `CMD`, `SD3`, `SD2` … on one side; `CLK`,
`SD0`, `SD1` … on the other) that identifies it as the **38‑pin
"ESP32 DevKit v1 / NodeMCU‑32S" style board** — the giveaway is that the flash
bus pins are broken out at all, which the 30‑pin DevKitC does not do.

Header layout, top → bottom:

| Left column | Right column |
|---|---|
| `3V3 EN 36 39` **`34 35 32 33 25 26 27`** `14 12 GND` **`13`** `SD2 SD3 CMD V5` | `GND 23 22 TX RX 21 GND` **`19 18`** `5` **`17 16 4`** `0 2 15 SD1 SD0 CLK` |

Two consequences worth the layout: the **entire status group plus both sensors plus a
ground plus the button is one contiguous 11‑pin run down the left column**, and
the detail group is a 6‑pin run down the right column.

| Signal | GPIO | Notes |
|---|---|---|
| **status** RED | 32 | left column; output only in this design, ADC1 unused here |
| **status** YELLOW | 33 | " |
| **status** GREEN | 25 | **was** `config loaded` |
| **status** BLUE | 26 | **was** `WiFi connected` |
| **detail** 1 GREEN | 19 | right column |
| **detail** 2 GREEN | 18 | " |
| **detail** 3 YELLOW | 17 | " — note GPIO**5** sits physically between 18 and 17 and is **skipped** (strapping pin, boot‑time output glitch) |
| **detail** 4 YELLOW | 16 | " |
| **detail** 5 RED | 4 | " |
| Button | 13 | `INPUT_PULLUP`, other side to GND. Sits directly below a GND pin on the left column |
| onboard LED | 2 | **unchanged** — keeps Story 2.4 / FR‑5 semantics verbatim, so the requirement is satisfied with the panel absent, disabled, or asleep |

GPIO16/17 are safe here **because this is a WROOM module**. On a WROVER they
are wired to the PSRAM chip and must not be used — worth re‑checking if the
board is ever swapped.

> ### ⚠️ GPIO6–11 are physically exposed on this board
>
> `CLK / SD0 / SD1 / SD2 / SD3 / CMD` at the bottom of both columns are the
> **SPI flash bus**. They look like six free pins and they are not: anything
> connected there crashes the chip or corrupts flash. Two specific traps:
>
> - **`V5` is immediately adjacent to `CMD` (GPIO11)** — and `V5` is exactly
>   where the battery pack's `+` wire lands (C10). One stray strand between
>   them puts 6.4 V onto a flash pin.
> - **The button on GPIO13 is four pins above `V5`**, with `SD2/SD3/CMD` in
>   between. Route that wire away from the bottom of the header, not along it.

Keeping GPIO2 unchanged is now a convenience rather than a compatibility
obligation: it costs nothing, it is the one indicator that needs no soldering,
and it is genuinely useful on the bench. But given C9, the honest reading is
that **FR‑5's "the Station signals a lost connection" is served for the first
time by this panel's red lane** — the onboard LED has been dutifully blinking
that signal at nobody since Story 2.4, and inside a sealed enclosure it will
continue to. Treat GPIO2 as a bench aid, and the red lane as the requirement.

Verified unclaimed: `main.cpp` uses 2/25/26/27/34, `Wire` uses 21/22, nothing
else in `src/` touches a GPIO. Ten pins total for the panel (9 LEDs + button),
out of roughly twenty free.

Deliberately avoided: **GPIO12** (must read low at boot — an LED is *probably*
harmless there, but "probably" is not a reason to use a strapping pin);
**GPIO0/2/5/15** (strapping); **GPIO6–11** (flash); **GPIO34–39** (input only).
GPIO13 is not a strapping pin and has no boot‑time output glitch (GPIO14 does —
it would flash an LED at every reset, which is why the button is not there).

Pin assignments are compile‑time constants, overridable without editing source
via `build_flags = -DGUSTIK_PANEL_PIN_RED=32` etc.

### 4.2 Current‑limiting resistors — 330 Ω on all nine

Active high: `GPIO → R → LED anode`, `cathode → GND`, so
`digitalWrite(pin, HIGH)` lights it.

**All nine LEDs use 330 Ω.** It is the only value on hand, and all four colours
have been confirmed to light visibly with it.

Colour count for the layout in §3 — inside the "up to 5 of each" ceiling (C7):

| | red | yellow | green | blue | total |
|---|---:|---:|---:|---:|---:|
| status group | 1 | 1 | 1 | 1 | 4 |
| detail group | 1 | 2 | 2 | 0 | 5 |
| **total** | **2** | **3** | **3** | **1** | **9** | The LEDs' own note
gives Vf 2.0–2.4 V, and the blue was measured at 2.6 V:

| Colour | Vf | I at 330 Ω from 3.3 V |
|---|---|---|
| Red / yellow / green | 2.0–2.4 V (per the kit's note) | 2.7–3.9 mA |
| Blue | 2.6 V (measured) | 2.1 mA |

`I = (3.3 V − Vf) / 330 Ω`. Every LED sits far inside the ESP32's 12 mA
recommended per‑pin figure (40 mA absolute maximum). One value everywhere means
no chance of fitting the wrong resistor to the wrong position, which is worth
more here than an optimal per‑colour match.

**Nine LEDs cost almost nothing over four**, because the detail group is a dot:
worst case is four status lanes solid (~12 mA) plus one detail LED (~3 mA) =
**~15 mA**, against the ~13 mA the original four‑LED budget assumed. §4.6 is
unchanged in substance.

**The high‑Vf green trap does not apply to this kit.** Bright‑green InGaN LEDs are
electrically blue dice and run at Vf ≈ 2.9–3.3 V, leaving almost nothing across
the resistor from a 3.3 V rail; these are 2.0–2.4 V parts and behave normally.
Keep it in mind only if a lane is ever replaced from a different batch — the
symptom is one colour visibly dimmer than the rest at the same resistor value,
and the check is the same one used on the blue here (LED in series with 1 kΩ
across 3.3 V, measure across the LED itself).

**Still unverified: daylight.** 2–4 mA is modest, and "visible on a bench" is
not "visible on the water at midday" — see §4.5. Check it outdoors before the enclosure is closed up. There
is headroom if a lane needs more punch: 220 Ω gives 4–6 mA and 150 Ω gives
6–9 mA, both still well inside the per‑pin limit. Prefer shading the panel over
raising the current — it is the cheaper fix and it costs no battery.

### 4.3 Wiring

38‑pin ESP32 DevKit v1. Left column carries the status group, right column the
detail group; both share the breadboard's ground rail.

```
      LEFT COLUMN (EN/RST side)                RIGHT COLUMN
   ┌──────────────────────────────┐    ┌──────────────────────────────┐
   │ ...                          │    │ ...                          │
   │ 34 ── vane        (existing) │    │ 21 ── SDA         (existing) │
   │ 35 ── battery sense  (§4.7)  │    │ GND ─────────────────┐       │
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
          │
         GND

   ▶|  = LED, arrow points from anode (GPIO side) to cathode (GND side)
```

Physical arrangement is **one flat row of nine**, in the §3 order
`R Y G B ⎢ G G Y Y R`, with a visible gap between the groups. The row's order is
a wiring choice, not a pin‑order constraint — jumpers can cross freely, and the
ZY‑204 breadboard (64 rows) has room to spare (§4.5).

**Fitting fewer than nine LEDs is supported**, and this is a tested invariant
(§7.3), not a hope. In priority order: **status RED** (faults — the only one
that says something is wrong), **status BLUE** (alive/heartbeat, the bug‑030
detector), **status GREEN** (data is landing), **status YELLOW** (radio), then
the detail group. With the status group alone you still get every fault code
and proof of life, and the button simply has nothing to show. With the detail
group partly fitted, unfitted positions are dark — the dot is still readable by
position for the ones present. No code change in any of these cases.

### 4.4 Button alternatives

**A button is in scope.** The alternatives below are kept for Story 5.2.

> **No switch of any kind is on hand yet (C10).** A button is improvisable on
> the breadboard in the meantime: poke a bare jumper into GPIO13's row, leave
> the far end loose, and tap it against the ground rail. The 30 ms debounce in
> §6 handles the bounce and it is electrically identical to a momentary switch
> — enough to exercise every mode on the bench and to decide whether the modes
> justify a real button before buying one.

- **No button fitted.** `INPUT_PULLUP` reads HIGH forever → no events → the
  panel stays in the default mode (wind). Fully supported, and still the
  behaviour if the button is left off or fails.
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

- Leave the LEDs on the breadboard, pointing up. **Space is not a constraint:**
  the board is a **ZY‑204, 64 rows × 20 columns plus 4 power rails**, so the
  38‑pin module, nine LEDs in a row, nine resistors, the button and the §4.7
  divider all fit with room over.
- The daylight question from §4.2 largely goes away: 2–4 mA is fine in shade,
  and inside a box everything is shade. Still worth a glance outdoors.
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

### 4.6 Power budget

The supply is **4 × AA, ~3000 mAh nominal** (C5), feeding the board's `V5`/VIN
pin through its onboard AMS1117 linear regulator. Against a pack that small the
panel's share is worth counting.

| State | Draw | Over a 10 h race day | Share of 3000 mAh |
|---|---|---|---|
| ESP32 with Wi‑Fi (existing baseline) | ~80–160 mA avg | 800–1600 mAh | 27–53 % |
| Panel awake, typical (2 status lanes + 1 detail dot) | +9 mA | +90 mAh | **3.0 %** |
| Panel awake, worst case (4 status solid + 1 dot, 330 Ω) | +15 mA | +150 mAh | **5.0 %** |
| Panel asleep (60 ms pulse / 10 s) | +0.02 mA | +0.2 mAh | ~0 |
| Panel hard off | 0 | 0 | 0 |

**Conclusion: keep the 300 s sleep default.** Against 3000 mAh a panel left
awake all day costs 3–5 % of the pack — small, but not noise. Sleeping makes it
zero while preserving the "dark = dead" rule.

**Three things about this supply that are not LED concerns but affect whether
the station survives the day** — recorded here because the numbers above are
meaningless if the pack cannot deliver them, and none of it is currently
written down anywhere:

- **No buck converter is available, and none is coming for v1.** The AMS1117
  stays, which makes this budget final rather than provisional. Runtime is `usable mAh / average mA` regardless of pack voltage,
  so the regulator's ~45 % loss shows up as **heat, not lost hours** — but it
  also means there is no cheap runtime win left to find, and Story 5.1's
  endurance test is now the only way to know whether the pack covers a race
  day. Rough expectation, to be measured and not trusted: at ~100–130 mA
  average, alkaline AA to the AMS1117's ~1.1 V/cell cutoff plausibly gives
  **11–16 h** — probably enough for 10 h, without much margin, and the
  regulator quits before the cells are empty. (A ~€2 buck into `VIN`, or
  straight to `3V3` bypassing the AMS1117, would roughly double it. Kept on
  record for a later season, not for this one.)
- **The regulator will run hot in a cardboard box.** 3.1 V dropped at ~120 mA
  is **~0.37 W** in a SOT‑223 package with only the devkit's copper to spread
  it — 30–40 °C above ambient in still air. Not dangerous, but do not pack the
  board in foam or bubble wrap: give the box a couple of holes and keep the
  regulator end clear. It falls to ~0.2 W as the pack approaches the knee.
- **There is no power switch (C10)**, so "off" means pulling the pack's wire
  out of the breadboard, and that is also how the Station is power‑cycled. Two
  consequences: the §4.7 divider draws nothing when the pack is unplugged
  (which is what removes the standby‑drain problem entirely), and a marginal
  breadboard contact in the pack's `+` path becomes a real failure mode — see
  the brown‑out note in §5.1. Seat the pack leads in the power rails rather
  than a single tie‑point column, and consider doubling both leads: two
  contacts in parallel halve the resistance for the cost of two jumpers.
- **Alkaline, not NiMH, for this wiring.** 4 × NiMH is 4.8 V nominal and sags
  to ~4.4 V under load, right at the AMS1117's dropout — the 3.3 V rail would
  brown out with most of the charge still in the cells. 4 × alkaline starts at
  ~6.4 V and stays above dropout until roughly 1.1 V/cell.
- **Expect 3000 mAh nominal to deliver noticeably less.** That figure is rated
  at a low discharge current; at ~120 mA continuous, alkaline AA cells give
  meaningfully less, and the regulator cuts out before the cells are empty.
  Plan on measuring a real run rather than trusting the label — that is Story
  5.1's endurance test.

### 4.7 Battery sense (Q16) — phase 6

**Built as an independent phase after the panel works.** The supply leaves a
gap: **4 × AA has no gauge.** Dry cells give no warning at all, and the first
symptom of a flat pack is the station going silent mid‑regatta. The panel is already the thing an operator looks at, so
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

**Three 10 kΩ resistors, and no capacitor** — neither is negotiable, both are
what is on hand. With no cap the *arrangement* matters: put the two spare
resistors **in parallel on the bottom leg**, not in series on the top.

| Arrangement (top leg / bottom leg) | Ratio | 6.6 V reads | Source impedance | Draw |
|---|---|---|---|---|
| **10 kΩ / 2 × 10 kΩ parallel (5 kΩ)** ← **chosen** | 0.333 | 2.20 V | **3.3 kΩ** | 427 µA |
| 2 × 10 kΩ series (20 kΩ) / 10 kΩ | 0.333 | 2.20 V | 6.7 kΩ | 213 µA |

Both give the same ratio from the same three resistors, but the first halves
the impedance the ADC's sample‑and‑hold has to charge — which is most of what a
100 nF cap from tap to GND would have bought. The extra 214 µA is 2.1 mAh over
a 10 h day, 0.07 % of the pack, so the trade is one‑sided.

**Do not use two equal resistors** — ratio 1/2 puts 6.6 V at 3.3 V, past the
ADC's range and past the pin's rating.

**Without a cap, filtering is entirely the firmware's job.** The cap would also
have smoothed the Wi‑Fi TX sag; the EMA and 3‑cycle confirmation below now
carry that alone, which is what they were specified for anyway. Expect a
noisier raw reading, and calibrate the ratio against a multimeter on a settled
value rather than a single sample. Adding 100 nF later is a strict improvement
and needs no code change.

**The divider costs nothing when the pack is unplugged**, because there is no
switch (C10): pulling the pack's wire disconnects the divider with it.

**Ceiling: this divider cannot represent more than ~7.35 V.** ADC1 at 11 dB has
a usable range of roughly 150–2450 mV, so 2.45 V / 0.3333 ≈ 7.35 V is the
highest pack voltage that reads back truthfully; above that the ADC saturates
and reports ~7.3 V, i.e. "healthy". A fresh alkaline pack at 6.4 V reads 2.13 V,
clear of it — but note this **kills the naive "> 8.0 V ⇒ implausible" check**,
which is unreachable through this divider. The upper implausibility test is
therefore expressed as **"raw reading pinned at full scale"**, which detects
saturation regardless of ratio, rather than as a voltage threshold.

**The eFuse VRef calibration is present on this chip** — confirmed by probe
(`Features: … VRef calibration in efuse`). So
`analogReadMilliVolts()` has a real calibrated reference, ~±3 % rather than
~±10 %, which is comfortable against thresholds 400 mV apart.

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
  computed pack voltage below 3.0 V, or a raw ADC reading pinned at full scale
  (see the ceiling note above), means the divider is not fitted, is miswired,
  or the pin is floating. This is §7.3's "absent hardware is a no‑op" invariant
  applied here, and it is what keeps a half‑built board from flashing a
  permanent low‑battery alarm.

#### The USB caveat — known, and accepted

The divider is on **`V5`** — there is no practical way to attach probes to the
pack itself, so tapping the pack directly is not available (C11). `V5` is also
the net USB feeds, which means the reading is of *the rail*, not of the pack:

| Source | `V5` reads | Which band |
|---|---|---|
| Fresh 4 × AA | 6.0–6.4 V | ok |
| Pack at the knee | 5.0 V | **warn** |
| **USB (through a protection diode)** | **~4.6–4.8 V** | **warn, sometimes critical** |
| USB (no diode) | ~5.0 V | **warn** |

**So a USB‑powered board will read `warn`, sometimes `critical`, and §5.10's
override will fire every 120 s.** That is a real cost — the fastest way to ruin
an alarm is to let it cry wolf — and it is **accepted on purpose**.

The reasoning, recorded so it is not relitigated: thresholds cannot separate
these cases (a nearly‑flat alkaline pack and a USB rail genuinely read the same
voltage), so the only alternatives were to detect the *battery* by some
side‑channel — a latch on "has the rail ever read above 5.4 V", say — or to
accept the false alarm. **Every such scheme adds a second, invisible state that
can itself be wrong**: a pack that boots already part‑used never arms, and then
the alarm is silent exactly when it is needed. Given that

- USB power only ever happens on the bench, where a person is already looking
  at a serial console, and
- **in the deployed configuration USB is simply not present**,

a loud panel on the bench is the cheaper failure than a quiet one on the water.
The rule stays "this measures the rail, and says what it sees".

**Silencing it on the bench**, when it becomes annoying, needs no new mechanism:

- `-DGUSTIK_PANEL_BATTERY=1` is already opt‑in at build time — a bench build
  simply omits it, and none of §4.7 exists. This is the intended workflow.
- A **long press** (hard off, §5.7) silences the whole panel including the
  override, and is runtime‑only, so a power cycle brings it back.
- `leds.enabled=false` in `config.txt` for a durable quiet.

The one thing that must **not** be done in response is widening the `unknown`
band to swallow the USB range — that would also swallow a genuinely flat pack,
which is the whole point of the feature.

#### Thresholds

Pack voltage, not cell voltage, measured under load:

| State | Pack | Per cell | Meaning |
|---|---|---|---|
| ok | ≥ 5.0 V | ≥ 1.25 V | fine |
| **warn** | 4.6 – 5.0 V | 1.15 – 1.25 V | the alkaline knee is near — have spares to hand |
| **critical** | < 4.6 V | < 1.15 V | AMS1117 dropout is 3.3 V + ~1.1 V ≈ 4.4 V; brown‑out is minutes away |
| unknown | < 3.0 V, or ADC pinned at full scale | — | divider absent or miswired; indicate nothing |

> On USB the rail lands in `warn` (occasionally `critical`) and is reported as
> such. That is the accepted caveat above, not a bug to be worked around.

> Voltage under load is a **rough** proxy for remaining charge on alkalines,
> and it recovers when the load drops. Treat the bar as "roughly how worried to
> be", never as a fuel gauge — and say so in the manual.

#### Indication

Deliberately *not* a ninth fault code — the eight‑code cap in §5.3 exists
because counting past eight on a moving boat does not work. Instead:

- **A global override** (§5.10): all nine LEDs fast‑blink in unison for 2 s,
  in any mode including sleep. **Every 120 s at `warn`, every 30 s at
  `critical`** — same unmistakable signal, two rates for two urgencies, no
  counting and no new vocabulary.
- **A battery detail mode** (§5.9), present in the button's cycle only when
  the flag is on.

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
of phase 6. Noted here so it is not forgotten. With no buck converter coming
(§4.6), this is now the cheapest way to learn anything real about runtime, and
worth pulling forward.

### 4.8 The steel plate — the panel is harmless, the ordering is not

The board and the magnetometer are permanently mounted on one breadboard on a
**steel plate** (C12). That is a magnetic decision, not an electrical one, and
it was made deliberately — but it constrains this design in one way and needs
recording in another.

**The panel's own currents are irrelevant.** This was worth checking, since the
panel is the one thing being added near a magnetometer:

| Source | Field at the magnetometer | vs Earth's ~50 µT | Heading error |
|---|---:|---:|---:|
| One LED, 3 mA at 3 cm | ~20 nT | 0.04 % | ~0.02° |
| Status group solid, 15 mA at 1 cm | ~300 nT | 0.6 % | ~0.35° |
| Supply current, 120 mA at 2 cm | ~1200 nT | 2.4 % | ~1.4° |

All of it is far inside a 22.5° octant, so the panel can be wired wherever it
is convenient. The supply current is the largest contributor and is a steady DC
offset that hard‑iron calibration absorbs anyway; twisting the pack's `+` and
`−` leads together cancels most of it for free.

**Calibration ordering becomes a hard rule.** Hard‑iron offsets describe the
*whole assembly*, so `mag_calibrate` must run **after** the LED panel, the
button and the battery divider are in their final positions, and nothing may be
rerouted afterwards without redoing it. This moves calibration in the rollout
(§13) from "before the panel" to "after phase 3, and again after any rewiring".

**Soft iron is the part calibration will not fix.** A steel plate does not just
add a fixed offset, it *distorts* the field direction‑dependently — the raw XY
trace becomes an ellipse rather than an offset circle. `mag_calibrate --tumble`
computes offsets only, so it corrects hard iron and leaves the ellipse. The
residual error varies with heading, is worst near 45° to the plate's axes, and
can plausibly exceed 22.5° if the plate is close and large. Cheap check with no
new code: after calibrating in place, look at the captured XY scatter — visibly
elliptical rather than circular means soft iron worth correcting, and the fix is
a per‑axis scale factor, two more config keys in the same shape as the offsets.

**And a warning for the manual:** §5.5's magnetometer lane reports "the I2C read
succeeded". A plate‑distorted heading passes that test perfectly. **Sensor mode
proves the sensor is alive, never that the heading is right** — say so, or a lit
yellow lane will be read as "direction is trustworthy".

---

## 5. Encoding

> **Orientation.** §5.2 and §5.3 are the status group, which never changes.
> §5.4–§5.6 and §5.9 are the four things the detail group can show. Subsection
> numbers are kept stable for cross‑references and are **not** the button's
> cycle order — that is **wind → signal → sensors → wind**, with battery
> inserted before the wrap once phase 6 exists (§6). Wind is the default, so
> §5.6 is the one to read first.

### 5.1 Boot self‑test

At `setup()`: all nine LEDs on for 400 ms, then a 100 ms sweep **left to right
across the whole row** (status R→Y→G→B, then detail 1→5), then the panel enters
its default state. Total ~1.3 s, no blocking (it runs as a normal panel state
while `setup()`'s existing work proceeds — see §8).

This makes a dead LED, a cold solder joint, or a wrong resistor obvious at
power‑on, and gives the person a positive "it just started" signal. The
left‑to‑right sweep does a second job: it teaches the row's order, so the
detail group's direction is learned without reading anything.

It also earns a third job for free: **a self‑test that keeps repeating means
the station is rebooting.** But note what changed with C10 — with the pack
connected by a jumper into a breadboard (C10), flat cells are not the first
suspect:

> **Repeating self‑test ⇒ check the power wire is seated, then check the
> cells.** At ~120 mA a marginal or worn breadboard contact drops real voltage
> exactly where the AMS1117 has no margin left, and an intermittent one
> produces brown‑out reboots indistinguishable from a flat pack. The cheaper
> test comes first. (On USB, neither applies — a repeating boot there means
> something else entirely.)

### 5.2 The status group — always on, never switched

These four lanes show the same thing at all times. They are **not** a mode:
nothing the button does affects them, so the panel can never be in a state
where a fault is invisible because someone went to look at the wind.

| Lane | Solid | Slow blink | Fast blink | Off |
|---|---|---|---|---|
| 🔵 **BLUE** — life | — | — | — | **loop hung or no power** |
| | one pulse per sample cycle (3 s) = loop alive · **double** pulse = flash buffer non‑empty (Q10) | | | |
| 🟢 **GREEN** — data landing | last POST stored ≥ 1 row | 2xx but stored **nothing** (Q9) or a backend too old to report counts | — | last send failed |
| 🟡 **YELLOW** — radio | associated, RSSI ≥ −67 dBm | associated but weak, RSSI < −75 dBm | scanning / connecting | not associated (Q4) |
| 🔴 **RED** — fault | fatal, station cannot work at all | *(not used)* | — | no fault |
| | otherwise blinks the **fault code**, §5.3 | | | |

> **As built, yellow's "fast blink = scanning / connecting" is not
> implemented, deliberately.** This firmware connects synchronously inside
> `send()`, so there is no scanning state to report — inventing one would be
> a lie told in LEDs. Not associated is simply *off*, per Q4. If the connect
> path ever becomes non‑blocking, the state is there to fill in.

The two properties worth memorising:

- **The whole row dark ⇒ not running.** No power, or a crash before `setup()`
  finished. (This rule is what the sleep behaviour in §5.7 is designed to
  preserve, and what "hard off" deliberately breaks — see the warning there.)
- **Blue dark, anything else lit ⇒ the loop is hung.** The bug‑030 signature,
  which otherwise produces no symptom at all.

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
| **8** | a sensor is failing | Q12–Q14 | press the button to sensor mode to see which |

Eight codes is the cap on purpose: counting past eight flashes on a moving
boat does not work. Anything finer lives in sensor mode or on the status page.

### 5.4 Detail mode — Signal (Q5, Q6)

One dot, sliding toward red as the signal degrades. Thresholds are the same
ones `/status.html` already draws reference lines at, so the LED and the web
view never disagree.

| Position | Colour | RSSI |
|---:|---|---|
| 1 | 🟢 | ≥ −55 dBm — excellent |
| 2 | 🟢 | −55 … −67 dBm — good (`/status.html`'s −67 line) |
| 3 | 🟡 | −67 … −80 dBm — usable (its −80 line) |
| 4 | 🟡 | −80 … −90 dBm — marginal, expect dropouts |
| 5 | 🔴 fast‑blink | < −90 dBm or not associated |

Note this deliberately does **not** read like phone signal bars — there is no
bar length to interpret, only a coloured position, and green is at the *good*
end where a filled bar would have had it backwards. See §3 for why.

**Which network you are on** (Q5) rides on the same dot with no counting: a
**steady** dot means network 1 (`network1.*`, the shore/primary Wi‑Fi); a dot
that **pulses off briefly once a second** means network 2 — "pulsing means
you're on somebody's phone".

This is the mode to hold the panel in while anchoring, or while walking an AP
around the shore.

### 5.5 Detail mode — Sensors (Q12, Q13, Q14)

**This mode deliberately breaks the metaphor.** Sensors need three independent
indicators, not a scale, so here the detail group is used *positionally* and its
colours carry nothing. That is acceptable precisely because this is a
diagnostic entered on purpose, never the at‑a‑glance default — and the legend
(§11) labels the positions.

| Position | Meaning |
|---:|---|
| 1 | **one 30 ms pulse per anemometer reed closure, live.** Dark while the cups are turning ⇒ the anemometer is not wired (bug‑059, exactly). Doubles as a wind‑speed‑as‑blink‑rate readout: ~1 pulse per revolution, so ~4 Hz at 5 m/s |
| 2 | vane: solid = ADC inside a known detent band · slow blink = outside every band (open or shorted wiring) |
| 3 | magnetometer: solid = last I2C read succeeded · slow blink = failing, headings are stale |
| 4 | *(unused, reserved)* |
| 5 🔴 | dark if all three are fine, slow blink otherwise — the one position whose colour still means what it means everywhere else |

This is the mode that would have turned bug‑059 from a multi‑hour investigation
into "spin the cups, watch position 1".

> **What this mode does not prove.** Position 3 reports that the I2C read
> succeeded, nothing more. A magnetometer distorted by the steel plate (§4.8)
> passes it perfectly. Alive ≠ correct.

### 5.6 Detail mode — Wind (Q15) — **the default**

This is what the detail group shows when nobody has touched the button, which
means the resting panel answers both "is it working" (status group) and "how
windy is it" (detail group) with no interaction at all.

One dot, on the boundaries `backend/src/static/beaufort.js` already uses — so
the LEDs, the dashboard and the Beaufort label all agree:

| Position | Colour | Speed | Beaufort |
|---:|---|---|---|
| 1 | 🟢 | < 1.6 m/s | 0–1 — bezvětří / vánek |
| 2 | 🟢 | 1.6 – 3.4 | 2 — slabý vítr |
| 3 | 🟡 | 3.4 – 5.5 | 3 — mírný vítr |
| 4 | 🟡 | 5.5 – 8.0 | 4 — dosti čerstvý vítr |
| 5 | 🔴 | ≥ 8.0 | 5+ — čerstvý vítr and above |
| 5 | 🔴 fast‑blink | ≥ 10.8 | 6+ — silný vítr, the capsize‑risk cue for P550s |

Here the severity gradient is doing real work rather than being a convention:
**green really does mean sailable and red really does mean the capsize cue**,
which is the panel's one genuine safety signal.

Where five positions are not enough, solid versus slow‑blink on the same dot
doubles the vocabulary without another LED — held in reserve, not used yet.

**Direction**, once on entry to this mode and then every 5 s: the dot goes dark
and **status YELLOW** blinks `octant + 1` times — 1 = S, 2 = SV, 3 = V, 4 = JV,
5 = J, 6 = JZ, 7 = Z, 8 = SZ (Czech, matching `compass.js`; octant indices as
defined by `correct/wind_direction.h`). It only matters when the network is
down — which is exactly when the panel matters most.

> Borrowing the status group's yellow lane for a few seconds is the one place
> anything touches the status group. It is a deliberate, bounded exception: the
> direction code is the only quantity that needs more resolution than five
> positions, the alternative was a compass rose that was parked (§12), and
> the borrow is brief and self‑announcing. The fault lane is never borrowed.

### 5.7 Sleep, and hard off

- After `leds.timeoutSeconds` (**default 300 s**) with no
  button press, the panel **sleeps**: everything dark except a blue
  proof‑of‑life pulse every 10 s and, if a fault is active, its red code
  repeated every 10 s instead of every 2 s.

  > 300 s is a usability choice, not a power one: at ~20 operator interactions
  > a day the difference between 120 s and 300 s is **~13 mAh, 0.4 % of the
  > pack** (§4.6). There was no battery argument either way, so the longer,
  > friendlier value wins.

- Any button press wakes it and restarts the timeout.
- A non‑default detail mode **auto‑returns to wind after 60 s**, so the panel is
  never found parked in "signal". This is now a convenience rather than a
  necessity — with the status group always visible, being parked in the wrong
  detail mode hides nothing important.
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

On entering detail mode N, **the detail group only** flashes together N times
(100 ms on / 100 ms off) before the mode's own display starts. Without it,
"which mode am I in?" is unanswerable, and it reuses the self‑test primitive so
it is nearly free.

Confining the banner to the detail group matters: a banner across the whole row
would look like the §5.10 low‑battery override, and the status group must never
flash for a reason that is not about status.

### 5.9 Detail mode — Battery (phase 6, `-DGUSTIK_PANEL_BATTERY=1` only)

One dot on the §4.7 thresholds. In the mode cycle only when the flag is on —
without it the cycle is wind → signal → sensors → wind and nothing here exists.

| Position | Colour | Pack | Per cell |
|---:|---|---|---|
| 1 | 🟢 | ≥ 5.6 V | ≥ 1.40 V |
| 2 | 🟢 | 5.2 – 5.6 V | 1.30 – 1.40 V |
| 3 | 🟡 | 5.0 – 5.2 V | 1.25 – 1.30 V |
| 4 | 🟡 | 4.6 – 5.0 V | 1.15 – 1.25 V — `warn` |
| 5 | 🔴 fast‑blink | < 4.6 V | `critical` |
| all five **slow**‑blinking | — | — | `unknown`: divider not fitted or miswired (§4.7) |

`unknown` gets its own pattern rather than showing position 5, because "no
divider" and "flat pack" must not look alike.

The dot reports **the rail**, so on USB it sits at position 4 or 5. Correct, and
expected — see §4.7's caveat.

### 5.10 Low‑battery override (phase 6)

The one signal that ignores modes entirely. **All nine LEDs** fast‑blink in
unison for 2 s, then the panel returns to whatever it was doing. Nine at once is
even less mistakable than four, and nothing else in the design lights the whole
row:

| State | Repeat | Also fires while asleep? |
|---|---|---|
| `warn` | every 120 s | yes |
| `critical` | every 30 s | yes |
| `ok` / `unknown` | never | — |

**This will fire on a USB‑powered bench board**, every 120 s, because the rail
reads inside the warn band. Known, accepted, and silenced by simply not setting
`-DGUSTIK_PANEL_BATTERY=1` in a bench build — see §4.7.

It fires **through sleep** on purpose: a pack going flat overnight or during a
long postponement is exactly when nobody is pressing buttons. It does **not**
fire when the panel is hard off (§5.7) — hard off means off, and it is runtime
only, so a power cycle restores it.

This is the only pattern in the design that lights all nine at once outside the
boot self‑test, which is what makes it unmistakable. The mode banner is
deliberately confined to the detail group (§5.8) so it can never be mistaken
for this.

---

## 6. Button behaviour

| Gesture | Effect |
|---|---|
| press < 30 ms | ignored (debounce) |
| **short press** (30 ms – 800 ms) | wake if asleep; otherwise advance the **detail group** wind→signal→sensors→wind (→battery→wind once phase 6 exists, §5.9). The status group is never affected |
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
them wholesale, and GPIO25/26 revert to their present `config loaded` /
`WiFi connected` behaviour. This is the answer to C1: if the panel does not
fit, it is one flag.

**Measured, not zero: +1 032 B.** Three small helpers the panel needs live
outside the flag on purpose (`vaneAdcPlausible()`,
`Anemometer::pulseCountSnapshot()`, the `leds.*` config keys) — see §9 for
why. The panel *itself* is gone; a kilobyte of shared plumbing is not.

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
`battery.warnVolts` and `battery.criticalVolts` on the same terms (§4.7). The
divider ratio in particular *must* be a config key rather than a constant,
because it is a measured property of three specific ±5 % resistors.

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
  panel.h/.cpp        pure   the mode machine: detail mode, sleep timer,
                             banner, scale mappings -> PanelOutputs
  button.h/.cpp       pure   debounce + short/long decoder ((level,ms)->event)
  battery.h/.cpp      pure   phase 6: mV -> pack volts -> EMA -> state machine
                             (ok/warn/critical/unknown) + dot position
  hw/led_panel.cpp    hw     pinMode/digitalWrite on the 9 pins
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
              PanelOutputs {
                  LanePattern status[4];   // red, yellow, green, blue
                  LanePattern detail[5];   // positions 1..5
              }
                       │
                       ▼
              hw/led_panel: 9 × digitalWrite
```

`PanelOutputs` is a flat array of patterns, not a "which dot is lit" index,
because §5.5 (sensors) drives the detail positions independently and §5.10
drives all nine at once. The **dot rule is a property of how the wind, signal
and battery modes fill that array**, not a constraint the renderer enforces —
which keeps the renderer trivial and every mode's own table under test.

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
counter, for the live pulses at detail position 1 (§5.5). 32‑bit aligned reads are atomic on
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

**Measured** (`pio run -e esp32dev`, after the phase‑0 partition change):

```
RAM:    15.8 %  (51 732 /   327 680 B)
Flash:  57.7 %  (1 210 461 / 2 097 152 B)   →  886 691 B free
```

**Measured cost, phases 1 + 2 as built** (estimate was 2–4 KB):

| Build | Flash | Δ vs the phase‑0 baseline | RAM |
|---|---:|---:|---:|
| baseline, before phase 1 | 1 210 461 B (57.7 %) | — | 51 732 B |
| `-DGUSTIK_STATUS_PANEL=0` | 1 211 493 B (57.8 %) | **+1 032 B** | 51 852 B |
| default (panel on) | 1 215 509 B (58.0 %) | **+5 048 B** | 52 020 B |

The panel itself is **~4 KB**, at the top of the estimate. §7.1's "exactly
zero bytes" turned out to be **+1 032 B**, and the difference is not the
panel: it is three small additions that live *outside* the build flag on
purpose — `vaneAdcPlausible()` (`sense/vane_decode.cpp`),
`Anemometer::pulseCountSnapshot()`, and the `leds.*` keys in
`parseStationConfig()`. Guarding the config parser in particular would mean a
`config.txt` carrying `leds.enabled` behaves differently depending on how the
firmware was built, which is a worse trade than a kilobyte out of 881 KB.

Phase 6's battery sense adds perhaps another 0.5–1 KB — one ADC read, an EMA
and a four‑state comparison — and is separately switchable.

That is ~0.6 % of the free space, so the ladder below is insurance, not a plan.

**Mitigation ladder, cheapest first:**

1. **Enlarge the app partition — done**, as phase 0. `firmware/partitions_gustik.csv`
   drops the never‑used second OTA slot: app 1.25 → 2 MB, filesystem
   1.375 → 1.875 MB, nothing given up. Reasoning, the table itself, the
   `spiffs`‑label trap and the reflash procedure are all in
   **`docs/hardware/flash-memory-map.md`**, which is also where the device's
   memory inventory lives.

2. **`-DGUSTIK_PANEL_STATIC=1`** — drop the pattern engine, lanes become plain
   on/off. Loses fault codes, the scale and the heartbeat; keeps the status
   group's four states. Saves perhaps 1–1.5 KB. This is the "eliminate blinking" fallback.
3. **Status group only** — drop the detail group and the button entirely.
4. **`-DGUSTIK_STATUS_PANEL=0`** — zero bytes, §7.1.

Whichever is chosen, **measure and record the before/after flash figure** in
the commit message, as this project already does.

> **Separate finding (bug‑060), not part of this feature, flagged because §9.1
> touches the same partition — and now arithmetic rather than suspicion.**
> `FlashBuffer` writes **one LittleFS file per reading** and
> `computeBufferCapacityForHours(4, 3)` asks for **4800** of them. Each record
> is ~110 B, which *is* below LittleFS's inline threshold, so the files do not
> each consume a 4 KB block — that part of the original worry was wrong. But
> every inline file's content, name and metadata tags live in the directory's
> metadata pairs, each costing two 4 KB blocks: 4800 entries at roughly 150 B
> of metadata is ~700 KB spread over ~175 pairs ≈ **1.4 MB**. That would never
> have fitted the old 1.375 MB filesystem, and fits the new 1.875 MB one with
> little margin.
>
> **Capacity is not the worse half.** LittleFS rewrites and compacts directory
> metadata as entries accumulate, so `push()` slows as `/buf` fills — against
> constraint C2, which says `loop()` must never block. Thousands of files in
> one directory is the shape of the problem.
>
> Still **not a measurement**: the cheap test is to leave the Station running
> with the backend unreachable and watch how long it actually buffers and how
> long `push()` takes as it fills. The fix, when it comes, is one file of
> fixed‑width records with the existing ring index — ~614 KB total and O(1)
> per write — and is unrelated to LEDs. See `docs/hardware/flash-memory-map.md`
> §4.

---

## 10. Testing

**Native (`pio test -e native`), ~25 new tests, no hardware:**

| Suite | Covers |
|---|---|
| `test_panel_pattern` | each pattern's duty cycle and phase; code‑N produces exactly N flashes then a pause |
| `test_panel_fault` | priority ordering; every fault in §5.3 reachable; no fault ⇒ code 0; the bug‑031 input (2xx, `inserted == 0`) ⇒ code 5; `hasCounts == false` never fabricates code 5 |
| `test_panel_modes` | short‑press cycling, banner on the detail group only, 60 s auto‑return to wind, sleep timeout, hard‑off toggle, `leds.enabled=false` ⇒ all nine always off. **The status group is byte‑identical across every detail mode** — the §3 invariant, asserted directly |
| `test_panel_scale` | RSSI banding at every boundary incl. exactly −67/−80; wind banding against `beaufort.js`'s boundaries; below‑minimum and absurd inputs are total. **Exactly one detail position lit** in wind/signal/battery modes — the dot rule, asserted rather than assumed, since §8.1's renderer does not enforce it |
| `test_button` | debounce, short vs long, a long press never also emits a short, no event when the pin is stuck HIGH (the "no button fitted" invariant) |
| `test_station_config` (extend) | `leds.*` parsing, defaults when absent, malformed values ⇒ defaults not garbage; phase 6 adds `battery.*` |
| `test_battery` *(phase 6)* | threshold boundaries and hysteresis; EMA rejects a single TX‑sag sample; 3‑cycle confirmation before a state change; implausible readings **and a full‑scale‑pinned ADC** ⇒ `unknown` and **no** indication (the "divider not fitted" invariant); positions at every boundary; `unknown` never renders as position 5. **No USB special‑casing anywhere** — a 4.7 V rail produces `warn` exactly like a 4.7 V pack does, asserted so nobody reintroduces a latch by accident (C11) |

**On hardware:** a `[env:panel_diag]` bring‑up sketch that walks all nine LEDs
one at a time in row order and prints one raw line per button transition
(`BTN <micros> <level>`) — dumb firmware, eyes as the analysis, per the standing
bring‑up preference. It exists to prove the *wiring*, not the logic: nine LEDs
means nine chances to swap two jumpers, and a walk in row order makes a
transposition obvious. Same `build_src_filter = -<*> +<...>` isolation as
`mag_diag` / `pulse_diag`.

**Manual checks on the real Station:** boot self‑test sweeps the whole row
left‑to‑right; pull the AP → red code 2 within one cycle; wrong token → code 4;
spin the cups → detail position 1 pulses in sensor mode; unplug the
magnetometer → position 3 blinks (and code 8 on the status red lane); cycle
every detail mode and confirm **the status group does not change**; leave it
5 minutes → sleeps to a heartbeat.

---

## 11. Documentation deliverables

1. **`docs/hardware/status-led-panel.md`** — the wiring, resistor table and Vf
   measurement method from §4, in the same shape as the existing
   `wind-sensor-wiring.md` / `magnetometer-wiring.md`.
2. **`manual.html` — a Czech section**, since that is what the person on the
   boat reads and the whole UI is Czech. Needs the status‑lane table, the
   fault‑code table, the detail‑group scale and the button gestures. Note the
   Czech‑abbreviation rule from `compass.js` applies to the direction blink
   code (`S` = *sever*, north — always spell the word out). Two caveats belong
   here and nowhere else: **the sensor mode proves a sensor is alive, not that
   it is right** (§4.8), and **the battery dot is a rough proxy under load, not
   a fuel gauge** (§4.7).
3. **The legend written where the LEDs are** — the fault codes are useless if
   they only exist on a web page you cannot reach when code 3 is flashing. For
   v1 that means **marker pen on the cardboard box** (C8), which cannot be lost
   and costs nothing; the printed card below is for Story 5.2's real enclosure.
   Either way the content is the same A7 block:

   ```
   ┌──────────────────────────────────────────────┐
   │  GUSTÍK — stavové diody                      │
   │                                              │
   │  VLEVO (4) — stav, svítí pořád:              │
   │   🔵 bliká = běží    🟢 svítí = data jdou    │
   │   🟡 svítí = wifi    🔴 bliká = chyba č.:    │
   │   1 chybí config     5 backend neuložil      │
   │   2 není wifi        6 ukládám do paměti     │
   │   3 backend neodpovídá   7 nesynchron. čas   │
   │   4 odmítnutý token  8 vadné čidlo (→ režim) │
   │                                              │
   │  VPRAVO (5) — měřidlo, svítí JEDNA:          │
   │   🟢🟢 = v pořádku  🟡🟡 = pozor  🔴 = mez   │
   │   normálně: SÍLA VĚTRU (🔴 = silný vítr!)    │
   │   tlačítko krátce = vítr → signál → čidla    │
   │   v režimu ČIDLA zleva: anemometr, korouhev, │
   │   kompas, –, 🔴 porucha                      │
   │                                              │
   │  všech 9 bliká naráz = SLABÉ BATERIE         │
   │  tlačítko dlouze = zhasnout                  │
   └──────────────────────────────────────────────┘
   ```

   The battery line is added by phase 6 (§5.10). It is worth the space even
   though it is the last thing built: it is the one signal that fires without
   anyone asking for it, so it is the one most likely to be seen by someone who
   has never read the legend.

---

## 12. Deferred

Everything else in this document is decided. Three items are parked with a
reason to revisit:

- **A 4‑LED compass rose.** It would show all eight octants with no counting,
  and it is the one idea here that would add product value rather than
  diagnostics. Two costs park it: it needs four *same‑colour* LEDs, or it puts
  a lit red LED on the panel meaning "north", and it duplicates §5.6's
  direction code. Revisit after phase 5 tells us whether anyone looks at the
  panel at all.
- **Button style for a sealed enclosure.** A plain push button is right for a
  cardboard box; the reed‑switch‑and‑magnet option (§4.4) is a Story 5.2
  decision and belongs with the enclosure.
- **A buck converter** (~€2) would roughly double runtime on the same cells
  (§4.6). Out of scope for v1 — none is available — but it is worth revisiting
  before a second season, because it changes what Story 5.1 is measuring.

---

## 13. Rollout

| Phase | Content | Verifiable by |
|---|---|---|
| **0 ✅ DONE** | partition table §9.1, as its own change | ✅ flash 92.4 % → **57.7 %**, built, flashed, `uploadfs`'d, `config.txt: 2 network(s) configured` confirmed over serial |
| **1 ✅ DONE** | `indicate/` pure modules + tests | ✅ `pio test -e native` **57 → 137 tests**, all passing |
| **2 ✅ DONE** | hw glue, `main.cpp` wiring, `leds.*` config keys, flag on by default | ✅ flash 57.7 % → **58.0 %** (+5 048 B); with `-DGUSTIK_STATUS_PANEL=0`, +1 032 B (see §9). Nothing soldered, so the Station is unchanged in the field |
| **3 ← NEXT** | 9 LEDs (330 Ω each) + button on the breadboard; `panel_diag`; manual checks §10 | by eye on the bench — no enclosure work needed for v1 (C8). Wiring reference: `docs/hardware/status-led-panel.md` |
| **3b** | **redo the magnetometer hard‑iron calibration** in the final wired position (§4.8) | `mag_calibrate` run with the panel fitted; XY scatter checked for softiron ellipticity |
| 4 | `manual.html` Czech section, `docs/hardware/status-led-panel.md`, legend on the box | reviewed on a phone |
| 5 | one regatta day of real use | does anyone actually look at it? |
| 6 | **battery sense** §4.7 / §5.9 / §5.10 — divider (3 × 10 kΩ), `battery.*` config keys, battery mode, low‑battery override | reported voltage tracks a multimeter across the pack; alarm fires on a deliberately run‑down set; divider unplugged ⇒ `unknown`, no indication. **On USB it alarms — that is the expected result, not a failure** |
| 7 | *(separate, backend)* ship pack voltage with each reading, graph it on `/status.html` | Story 5.1's endurance test becomes a chart |

Phases 1 and 2 are safe to land with the flag off — nothing changes on the
running Station until phase 3 puts real LEDs on real pins.

**Phase 3b is not optional and not reorderable.** Hard‑iron offsets describe
the whole assembly, and the panel adds wiring to a board bolted to a steel plate
(C12). Calibrating before the panel is fitted produces numbers that describe an
assembly that no longer exists — and nothing in the firmware or the dashboard
would show that they are wrong.
