# gustik-scripts

Bench-side Python tools for Gustik hardware. Currently: a driver for the
QMC5883P magnetometer (the chip on "GY-271" boards silkscreened HMC5883L
that answer at I2C address `0x2C`), used to correct wind direction for the
committee boat's yaw.

## Running

```sh
uv run python -m gustik_scripts --bus 18 --help
uv run python -m unittest discover -s tests -t tests    # 50 tests, stdlib only
```

`uv run` syncs and installs the project into `.venv`, so `gustik_scripts`
imports without `PYTHONPATH`. `-t tests` keeps `tests/` from needing to be
a package.

`--bus 18` is whichever `/dev/i2c-N` the MCP2221 USB bridge came up as;
check `ls /dev/i2c-*` after replugging, and note the number can change.

## Getting a usable heading

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
  they were captured at — a mismatch warns.
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
