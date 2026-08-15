# gustik-scripts

Bench-side Python tools for Gustik hardware. Currently: a driver for the
QMC5883P magnetometer (the chip on "GY-271" boards silkscreened HMC5883L
that answer at I2C address `0x2C`), used to correct wind direction for the
committee boat's yaw.

## Two ways in, and which one you want

There are two front ends to the same calibration maths:

| | `mag_calibrate` (ESP32, serial) | `gustik_scripts` (bench, I2C) |
|---|---|---|
| Talks to | the station's own ESP32 | a USB-I2C bridge (MCP2221) |
| Needs | nothing but `python3` | `smbus2`, a bridge, `/dev/i2c-N` |
| Calibrates | **the real assembly** | a bench rig |

**Prefer `mag_calibrate`.** A calibration describes one rigid assembly — the
ESP32, its breadboard, its base, every wire that turns with it — so a capture
taken through a USB bridge on a different board is a calibration of the wrong
thing. The bench tool remains useful for identifying a chip or checking wiring
before an ESP32 exists to plug it into.

## Calibrating the real station (`mag_calibrate`)

Flash the capture sketch, which streams raw counts over serial and does
nothing else:

```sh
cd ../firmware && ~/.platformio/penv/bin/pio run -e mag_diag -t upload
```

Then capture, turning the board through every orientation — spin it, roll it,
stand it on each edge — for the whole minute:

```sh
cd ../scripts
python3 -m gustik_scripts.mag_calibrate --tumble
```

It writes `qmc5883p-calibration.json`, saves the raw stream to
`mag-capture-<timestamp>.txt`, and prints two paste-ready blocks: config-file
lines and a C++ constant. Take the config-file route while the mount is still
likely to change — it needs only `pio run -t uploadfs`, not a reflash.

Finally, put the station firmware back:

```sh
cd ../firmware && ~/.platformio/penv/bin/pio run -e esp32dev -t upload
```

Useful extras:

```sh
# re-analyse an old capture, without touching the hardware
python3 -m gustik_scripts.mag_calibrate --from-file mag-capture-20260815-2130.txt

# confirm the mount end-to-end: turn the board clockwise through a full turn
python3 -m gustik_scripts.mag_calibrate --check-rotation --up=-z

# which axis is vertical? (needs a --tumble calibration, board held level)
python3 -m gustik_scripts.mag_calibrate --detect-up --axis z
```

Every capture is saved verbatim, and that is not housekeeping. The wind-vane
capture that confirmed one calibration later answered a question nobody had
asked when it was taken (`docs/hardware/wind-sensor-wiring.md`). Raw data can
be re-questioned; a derived number cannot.

### If it says NOT A ROTATION

It means the field barely moved, so the capture is of a stationary sensor. The
"12/12 sectors covered" line above it does **not** contradict that: sector
coverage is measured after rescaling each axis by its own span, so noise from
a motionless board scatters right around the circle and looks like a perfect
turn. Only the raw peak-to-peak span can tell the two apart. Offsets from such
a capture are just wherever the sensor was pointing, and would give a stable,
confident, wrong heading — the tool exits non-zero rather than let them look
usable.

## Running the bench tool

```sh
uv run python -m gustik_scripts --bus 18 --help
uv run python -m unittest discover -s tests -t tests    # 88 tests, stdlib only
```

The tests and `mag_calibrate` are stdlib-only and run under a bare `python3`
with `PYTHONPATH=src`; only the `--bus` I2C path needs `uv`/`smbus2`.

`uv run` syncs and installs the project into `.venv`, so `gustik_scripts`
imports without `PYTHONPATH`. `-t tests` keeps `tests/` from needing to be
a package.

`--bus 18` is whichever `/dev/i2c-N` the MCP2221 USB bridge came up as;
check `ls /dev/i2c-*` after replugging, and note the number can change.

## Getting a usable heading (bench tool)

The same four steps exist on the ESP32 side as flags on `mag_calibrate`; this
section is the `--bus` variant.

Order matters. A raw reading is Earth's field **plus** every magnet that
turns with the sensor, and that bias is routinely several times larger
than the signal — which puts the origin outside the circle the readings
trace, so `atan2` can only sweep a narrow arc. The symptom is a heading
stuck in a band of a few tens of degrees no matter how far you turn the
board. Calibration is not a refinement here; nothing works without it.

1. **Calibrate.** Tumble through every orientation — spin, roll, stand it
   on each edge:

   ```sh
   uv run python -m gustik_scripts --bus 18 --calibrate --tumble
   ```

   Written to `./qmc5883p-calibration.json` and loaded automatically
   afterwards. `--calibrate` without `--tumble` does a level spin
   instead: enough for heading, but it cannot measure the vertical axis
   (see below), so it will not satisfy step 2.

2. **Find which way is up.** Hold the board level in its mounting
   orientation. Pass the axis you know is vertical, so only its sign has
   to be worked out:

   ```sh
   uv run python -m gustik_scripts --bus 18 --detect-up --axis z
   ```

   Don't know which axis? A level spin's report names it: the axis that
   *doesn't* sweep is the one you turned about.

3. **Verify the mount.** Turn the board clockwise through a full turn:

   ```sh
   uv run python -m gustik_scripts --bus 18 --up=-z --check-rotation
   ```

   It must report 12/12 sectors and `clockwise`. If it says `MIRRORED`,
   flip the sign of `--up`. Write axis arguments with an `=`; argparse
   reads a bare `-z` as a flag.

4. **Read it.**

   ```sh
   uv run python -m gustik_scripts --bus 18 --up=-z --declination 5.5
   ```

   `--declination 5.5` converts magnetic bearings to true north for
   Czechia in 2026.

## Mounting

Describe the geometry instead of hunting for per-axis sign flips:

```python
QMC5883P(bus=18, orientation=Orientation(up="-z", forward="+x"))
```

`up` is the axis pointing away from the ground (`-z` for a board mounted
upside down), `forward` the axis whose bearing is reported. Rotation
handedness follows from those two — `left` is derived as `up × forward`,
so a wrong `up` sign mirrors the heading. Vertical mounts are supported:
name the axes to match, e.g. `Orientation(up="+y", forward="+z")`.

Heading assumes the forward/left plane is horizontal. This chip has no
accelerometer, so heel and pitch cannot be compensated for; a vertical
mount is fine as long as the board itself stays upright.

## Things that will bite you

- **A calibration describes one rigid assembly.** Move the sensor, change
  its mount, or change what sits beside it, and redo it. The offsets are
  in raw LSB, so they are also specific to the field range (`--range`)
  they were captured at — a mismatch warns. This is also why a bench
  calibration taken over a USB-I2C bridge does not describe the station:
  use `mag_calibrate` for anything that will actually be flashed.
- **The Y offset is flipped on the way into the firmware.** The station's
  mount is up=-z/forward=+x, so `sense/magnetometer.cpp` negates Y as it
  reads, and the offset subtracted from it must be negated to match. The
  JSON file keeps the chip's own frame; `mag_calibrate`'s paste-ready
  output has the flip already applied. Do it once, in one place, or the
  heading comes out mirrored about the north-south axis.
- **A level spin cannot calibrate the vertical axis.** With the board
  level, that axis is constant, so the "centre" found for it is Earth's
  vertical field *plus* the hard iron, inseparably. Subtracting it zeroes
  the vertical component rather than debiasing it. Harmless for heading,
  which ignores that axis — but it is why `--detect-up` insists on
  `--tumble`.
- **Don't trust the dip angle.** Nominal dip in Czechia is ~66°, which
  would make the vertical component the largest. Measured on this bench
  it was ~40°, with vertical *smaller* than horizontal, because of local
  soft iron. Detection therefore relies only on the *sign* of the
  vertical reading, never its magnitude.
- **The MCP2221 wedges.** One failed I2C read leaves its state machine
  stuck, so every later read times out while `i2cdetect` still looks
  fine. Unplug and replug it. Rotating the board can tug the wiring and
  trigger this, so leave slack. Captures return what they collected
  rather than discarding it, and re-check `/dev/i2c-N` after replugging.
