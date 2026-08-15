"""
mag_calibrate.py

Calibrate the magnetometer *as it is actually mounted*, through the ESP32.

The sibling tool (qmc5883p.py) talks to the chip over a USB-I2C bridge, which
means calibrating a bench rig rather than the station. A calibration describes
one rigid assembly - the ESP32, its breadboard, its iron base, every wire that
turns with it - so the only capture worth trusting is one taken through the
station's own hardware, in the station's own mount.

    ~/.platformio/penv/bin/pio run -e mag_diag -t upload   # from firmware/
    python3 -m gustik_scripts.mag_calibrate --tumble

Stdlib only: no smbus2, no pyserial. Run it with plain `python3` from
scripts/, no venv needed.

Redo this after any physical change to the assembly - a different breadboard,
a different base, a moved wire, the final enclosure.
"""

import argparse
import math
import os
import sys
import time

from .calibration import CalibrationError, MagCalibration, describe_spin
from .esp32_serial import (
    DEFAULT_BAUD,
    DEFAULT_PORT,
    SerialLineReader,
    capture_samples,
    read_capture_file,
)
from .firmware_output import (
    FIRMWARE_FIELD_RANGE,
    config_lines,
    cpp_constant,
    motion_warning,
    range_warning,
)
from .orientation import (
    Orientation,
    OrientationError,
    axis_letter,
    detect_up_axis,
    rotation_summary,
    up_axis_for,
)
from .report import print_calibration_report, print_rotation_report

DEFAULT_CALIBRATION_FILE = "qmc5883p-calibration.json"


def _build_parser():
    parser = argparse.ArgumentParser(
        prog="python3 -m gustik_scripts.mag_calibrate",
        description="Calibrate the magnetometer over the ESP32's serial stream "
                    "(flash firmware env `mag_diag` first).",
        epilog="Write axis arguments with an '=': argparse reads a bare -z as a flag.",
    )
    parser.add_argument("--port", default=DEFAULT_PORT,
                        help=f"serial port the ESP32 is on (default: {DEFAULT_PORT})")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--from-file", default=None, metavar="PATH",
                        help="re-analyse a saved raw capture instead of reading the "
                             "board; lets an old capture answer a new question")
    parser.add_argument("--raw-log", default=None, metavar="PATH",
                        help="where to mirror the raw serial stream "
                             "(default: mag-capture-<timestamp>.txt)")
    parser.add_argument("--cal", default=DEFAULT_CALIBRATION_FILE,
                        help="calibration JSON to write, and to load for the "
                             "--check-rotation / --detect-up modes")
    parser.add_argument("--range", dest="field_range", default=FIRMWARE_FIELD_RANGE,
                        help="field range the sketch configured; offsets are raw LSB, "
                             f"so this must match the firmware ({FIRMWARE_FIELD_RANGE})")
    parser.add_argument("--seconds", type=float, default=None,
                        help="capture duration (default: 60 tumbling, 30 spinning, "
                             "20 for --check-rotation)")
    parser.add_argument("--tumble", action="store_true",
                        help="capture every orientation - spin, roll, stand it on each "
                             "edge - instead of a level spin. Needed for --detect-up, "
                             "and the better default for a fresh mount.")
    parser.add_argument("--up", default="-z",
                        help="sensor axis pointing away from the ground; the station's "
                             "confirmed mount is -z (board upside down)")
    parser.add_argument("--forward", default="+x",
                        help="sensor axis whose bearing is reported")

    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check-rotation", action="store_true",
                      help="turn the board clockwise while this runs, to verify the "
                           "mount description end-to-end against an existing "
                           "calibration")
    mode.add_argument("--detect-up", action="store_true",
                      help="report which sensor axis is currently vertical; needs a "
                           "--tumble calibration and the board held level")
    parser.add_argument("--axis", default=None,
                        help="for --detect-up: the axis you already know is vertical "
                             "(e.g. z), so only its sign has to be determined")
    return parser


def _default_raw_log():
    return time.strftime("mag-capture-%Y%m%d-%H%M%S.txt")


def _progress(elapsed, total, count):
    print(f"  {elapsed:5.1f}/{total:.0f}s  {count} samples", end="\r", flush=True)


def _capture(args, seconds):
    """Collect raw samples, from the board or from a saved capture."""
    if args.from_file:
        samples = read_capture_file(args.from_file)
        print(f"{len(samples)} samples read from {args.from_file}")
        return samples

    raw_log_path = args.raw_log or _default_raw_log()
    try:
        reader = SerialLineReader(args.port, baud=args.baud)
    except OSError as exc:
        raise SystemExit(
            f"cannot open {args.port}: {exc}\n"
            "Is the board plugged in, and are you in the 'dialout' group? "
            "Flash the capture sketch first: pio run -e mag_diag -t upload"
        ) from exc

    with reader, open(raw_log_path, "w") as raw_log:
        samples = capture_samples(reader, seconds, raw_log=raw_log, progress=_progress)
    print(" " * 40, end="\r")

    if not samples:
        raise SystemExit(
            f"no samples arrived on {args.port} in {seconds:.0f}s.\n"
            "The mag_diag sketch prints lines like 'MAG 123 -456 789'. Check with:\n"
            f"  stty -F {args.port} {args.baud} raw -echo && timeout 3 cat {args.port}\n"
            "If that shows '# I2C ...' lines instead, the magnetometer wiring is the "
            "problem, not this script."
        )
    print(f"raw capture saved to {raw_log_path}")
    return samples


def _run_calibrate(args):
    if args.seconds is not None:
        seconds = args.seconds
    else:
        seconds = 60.0 if args.tumble else 30.0

    if not args.from_file:
        if args.tumble:
            print(f"Turn the board through EVERY orientation -- spin it, roll it, "
                  f"stand it on each edge, unhurriedly. Capturing for {seconds:.0f}s...")
        else:
            print(f"Rotate the board SLOWLY through at least two full turns, keeping "
                  f"it level. Capturing for {seconds:.0f}s...")

    samples = _capture(args, seconds)

    try:
        report = describe_spin(samples)
        calibration = MagCalibration.from_samples(samples, field_range=args.field_range)
    except CalibrationError as exc:
        print(f"Cannot calibrate from this capture: {exc}")
        return 1

    print_calibration_report(calibration, report)
    calibration.save(args.cal)
    print(f"Written to {args.cal}")

    _print_firmware_block(calibration, report)
    # A capture that never moved yields plausible-looking but meaningless
    # offsets, so it must not exit 0 - otherwise a scripted run would treat it
    # as a good calibration.
    return 1 if motion_warning(report, calibration.field_range) else 0


def _print_firmware_block(calibration, report=None):
    print()
    print("=" * 72)
    print("FOR THE FIRMWARE -- pick one of the two routes below.")
    stalled = motion_warning(report, calibration.field_range) if report else None
    if stalled:
        print(f"!! DO NOT USE THESE NUMBERS: {stalled}")
    warning = range_warning(calibration)
    if warning:
        print(f"!! {warning}")
    print()
    print("A) firmware/data/config.txt, then `pio run -t uploadfs` (no reflash;")
    print("   prefer this while the mount is still likely to change):")
    print()
    for line in config_lines(calibration):
        print(f"    {line}")
    print()
    print("B) firmware/src/main.cpp, replacing kMagnetometerCalibration, then")
    print("   `pio run -e esp32dev -t upload` (keeps the value under review):")
    print()
    for line in cpp_constant(calibration):
        print(f"    {line}")
    print("=" * 72)


def _load_calibration(args):
    if not os.path.exists(args.cal):
        raise SystemExit(
            f"no calibration at {args.cal} -- run without --check-rotation/--detect-up "
            "first to capture one."
        )
    calibration = MagCalibration.load(args.cal)
    print(f"Loaded calibration from {args.cal}")
    if calibration.field_range not in (None, args.field_range):
        print(f"WARNING: calibration was taken at range {calibration.field_range}, "
              f"but --range says {args.field_range}. Offsets are raw LSB and do not "
              "carry across ranges.")
    return calibration


def _run_check_rotation(args, orientation):
    calibration = _load_calibration(args)
    seconds = 20.0 if args.seconds is None else args.seconds

    if args.from_file:
        samples = _capture(args, seconds)
        headings = [orientation.heading(calibration.apply(s)) for s in samples]
    else:
        print(f"Turn the board CLOCKWISE (seen from above) through a full turn, "
              f"keeping it level. Recording for {seconds:.0f}s...")
        try:
            reader = SerialLineReader(args.port, baud=args.baud)
        except OSError as exc:
            raise SystemExit(f"cannot open {args.port}: {exc}") from exc
        headings = []
        with reader:
            start = time.monotonic()
            while True:
                elapsed = time.monotonic() - start
                if elapsed >= seconds:
                    break
                # Newest sample only: the heading has to track the board as it
                # is turned, not replay a queue of stale readings.
                sample = reader.latest_sample(timeout=1.0)
                if sample is None:
                    continue
                corrected = calibration.apply(sample)
                if math.hypot(*orientation.horizontal(corrected)) == 0.0:
                    continue
                headings.append(orientation.heading(corrected))
                _progress(elapsed, seconds, len(headings))
                time.sleep(0.05)
        print(" " * 40, end="\r")

    if len(headings) < 2:
        print("Not enough headings to judge a rotation -- was the board streaming?")
        return 1
    report = rotation_summary(headings)
    return 0 if print_rotation_report(report, headings, orientation) else 1


def _run_detect_up(args):
    calibration = _load_calibration(args)
    if not calibration.covers_all_axes:
        print(
            "Cannot determine the vertical axis from a calibration that did not sweep "
            f"{', '.join(calibration.unswept_axes)}: that axis's offset absorbed "
            "Earth's vertical field, so its calibrated reading is ~0 by construction. "
            "Recapture with --tumble."
        )
        return 1

    print("Hold the board LEVEL in its mounting orientation...")
    samples = _capture(args, 3.0 if args.seconds is None else args.seconds)
    corrected = [calibration.apply(s) for s in samples]
    mean = tuple(sum(axis) / len(corrected) for axis in zip(*corrected))
    try:
        if args.axis is None:
            up = detect_up_axis(mean)
        else:
            up = up_axis_for(args.axis, mean["xyz".index(axis_letter(args.axis))])
    except (CalibrationError, OrientationError) as exc:
        print(f"Cannot determine the vertical axis: {exc}")
        return 1
    print(f"Vertical axis appears to be: up={up!r} (pass it as --up={up})")
    return 0


def main(argv=None):
    args = _build_parser().parse_args(argv)
    try:
        orientation = Orientation(up=args.up, forward=args.forward)
    except OrientationError as exc:
        raise SystemExit(f"bad mount description: {exc}") from exc

    try:
        if args.check_rotation:
            return _run_check_rotation(args, orientation)
        if args.detect_up:
            return _run_detect_up(args)
        return _run_calibrate(args)
    except KeyboardInterrupt:
        print("\ninterrupted")
        return 130


if __name__ == "__main__":
    sys.exit(main())
