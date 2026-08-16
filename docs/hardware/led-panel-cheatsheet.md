# Status LED panel — operator cheatsheet

**Quick reference for the person on the boat.** Full reasoning and every edge
case live in `docs/superpowers/specs/2026-08-16-status-led-panel-design.md`;
wiring lives in `docs/hardware/status-led-panel.md`. This page is just: look
at the row, know what it means.

```
  R   Y   G   B  ┃  G   G   Y   Y   R
 └── status ────┘  └──── detail ─────┘
```

- **Left four (status)** — always on, never hidden by the button. If
  something's wrong, it's always showing here.
- **Right five (detail)** — one lit dot, sliding left (good) to right (bad).
  Defaults to **wind**. Short-press the button to cycle: wind → signal →
  sensors → wind.

**Whole row dark = station not running.** No power, or it crashed before
startup finished.

---

## 1. Status group (left four — always on)

| LED | Solid | Blinking | Off |
|---|---|---|---|
| 🔴 RED | — | **flashing a number = fault code**, see below | no fault (good) |
| 🟡 YELLOW | good signal | slow = weak signal | not connected to Wi-Fi |
| 🟢 GREEN | data is being saved | slow = server responded but saved nothing | last send failed |
| 🔵 BLUE | — | one pulse every 3 s = alive · **two pulses** = alive + backlog queued | **loop hung, or no power** |

**Blue dark, anything else lit → the loop is hung.** That's a real fault even
though nothing else says so.

## 2. Fault codes (red LED, count the flashes)

Flashes fast (150 ms), pauses ~1.2 s, repeats. Only the worst active fault
shows.

| Flashes | Meaning | Do this |
|---:|---|---|
| **solid** (not flashing) | fatal — didn't start up | power-cycle, then reflash |
| 1 | no config | re-run `uploadfs` |
| 2 | no known Wi-Fi network in range | check the hotspot is on, move closer |
| 3 | Wi-Fi is up, but can't reach the server | check the hotspot's mobile data |
| 4 | server rejected us (wrong token) | re-upload config |
| 5 | server said OK but saved nothing | reboot the station |
| 6 | buffering — last send failed, queuing to flash | usually transient; if it stays, check 2/3/4 |
| 7 | clock never synced | needs real internet, not just a local AP |
| 8 | a sensor is failing | press the button → sensor mode to see which |

## 3. Detail group (right five — button cycles the mode)

On entering a mode, the five detail LEDs flash together **N times** (N = mode
number below) so you know where you are, then settle into the mode's display.

### Wind (mode 1 — the default)

| Dot position | Colour | Wind |
|---:|---|---|
| 1 | 🟢 | calm |
| 2 | 🟢 | light |
| 3 | 🟡 | moderate |
| 4 | 🟡 | fresh |
| 5 | 🔴 solid | strong |
| 5 | 🔴 **fast-blinking** | **capsize-risk wind — the one genuine safety signal** |

**Direction**, once on entering this mode and then every 5 s: the wind dot
goes dark and **status YELLOW** blinks instead — count the flashes,
1 = S, 2 = SE, 3 = E, 4 = SE... (octant, clockwise). Mainly useful when the
network is down and the dashboard is unreachable.

### Signal (mode 2)

| Dot position | Colour | Meaning |
|---:|---|---|
| 1–2 | 🟢 | good |
| 3–4 | 🟡 | usable / marginal |
| 5 | 🔴 fast-blink | very weak or not connected |

A **steady** dot = on the primary network. A dot that **pulses off briefly
once a second** = on the backup network ("pulsing means you're on somebody's
phone").

### Sensors (mode 3 — diagnostic, positions don't follow the colour scale here)

| Position | Meaning |
|---:|---|
| 1 | pulses once per anemometer rotation — **dark while the cups are turning = wind sensor not wired** |
| 2 | wind vane: solid = OK, slow blink = wiring problem |
| 3 | compass/magnetometer: solid = OK, slow blink = failing |
| 4 | (unused) |
| 5 🔴 | dark = everything fine, slow blink = something above is bad |

---

## 4. The button

| Press | Effect |
|---|---|
| short (tap) | wake the panel if asleep; otherwise cycle detail mode: wind → signal → sensors → wind |
| long (hold ≥ 2 s) | turn the whole panel off / back on |

- Leaving a non-default mode alone for 60 s auto-returns to wind.
- No button press for 5 minutes → panel **sleeps**: goes dark except a slow
  blue pulse (and the fault code, if any, at a slower repeat). Any press
  wakes it.
- Long-press "off" is **temporary** — it resets itself on the next power
  cycle or reboot. To turn it off for good, that's a config change
  (`leds.enabled=false`), not the button.

## 5. What to actually watch for on race day

1. **Row goes dark** → station stopped. Check power first.
2. **Self-test (all nine flash, then sweep once) keeps repeating** → the
   station is rebooting in a loop. Check the power connection is seated
   before suspecting the battery pack.
3. **Blue dark, others lit** → hung, needs a power-cycle.
4. **Red flashing** → read the count, use the fault table above.
5. **Wind dot fast-blinking red** → capsize-risk wind, brief the crews.
