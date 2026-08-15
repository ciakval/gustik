"""
qmc5883p.py

Minimal Python driver for the QMC5883P 3-axis magnetometer over I2C,
using the smbus2 library.

This chip is commonly found on "GY-271"-style breakout boards that are
silkscreened "HMC5883L" but actually enumerate at I2C address 0x2C.
It is NOT register-compatible with the real HMC5883L (0x1E) or the
QMC5883L (0x0D) -- this driver targets the QMC5883P register map only.

Hardware note:
    A regular laptop has no native I2C bus. You need a USB-to-I2C
    bridge (e.g. an FT232H breakout, a CH341 adapter, or a
    microcontroller running an I2C-passthrough firmware). Once
    connected, Linux will expose it as /dev/i2c-N. Find your bus
    number with:

        ls /dev/i2c-*
        i2cdetect -l

MCP2221 note:
    The MCP2221's Linux HID driver has a known bug: after a failed I2C
    read, its internal state machine can get stuck, so *every*
    subsequent read (even to a healthy device) hangs or times out,
    while i2cdetect still appears to work (it only does quick address
    probes, not full reads). If reads stop working, first try
    physically unplugging and replugging the MCP2221 itself.

    To sidestep this as much as possible, every read in this driver is
    split into two fully independent I2C transactions (each with its
    own STOP condition) instead of a single combined write+repeated-
    start+read transaction, with a short settle delay and a retry.

Heading requires calibration -- it is not optional
    Raw readings include a constant bias from any magnetic material that
    turns with the sensor (PCB, connectors, breadboard clips). Once that
    bias exceeds the size of Earth's field, the heading gets stuck inside
    a narrow band and barely responds to rotation at all. See
    calibration.py for why, then run:

        python -m gustik_scripts --bus 18 --calibrate

    and rotate the sensor slowly through a couple of full, level turns
    when prompted. The result is written to ./qmc5883p-calibration.json
    and picked up automatically on later runs. Redo it whenever the
    sensor's mount or its magnetic surroundings change.

Mounting orientation
    Say how the board sits, instead of guessing at per-axis sign flips:

        QMC5883P(bus=18, orientation=Orientation(up="-z", forward="+x"))

    `up` is the sensor axis pointing away from the ground -- "-z" for a
    board mounted upside down -- and `forward` is the axis whose bearing
    heading() reports. Vertical mounts work too; name the axes to match.
    See orientation.py for the frame convention and the tilt caveat.

    To find `up` empirically you need a calibration that swept all
    three axes, so run the tumble variant, then hold the board level in
    its intended orientation:

        python -m gustik_scripts --bus 18 --calibrate --tumble
        python -m gustik_scripts --bus 18 --detect-up --axis z

    Then confirm the whole mount by turning the board clockwise:

        python -m gustik_scripts --bus 18 --up=-z --check-rotation

    (Write axis arguments as --up=-z; argparse reads a bare -z as a flag.)

    A level spin cannot do this: it holds the vertical axis constant, so
    that axis's offset and Earth's vertical field are inseparable.

Example:
    from gustik_scripts.qmc5883p import QMC5883P
    from gustik_scripts.orientation import Orientation

    with QMC5883P(bus=18, orientation=Orientation(up="-z")) as mag:
        x, y, z = mag.read_gauss()
        print(f"X={x:+.4f}G Y={y:+.4f}G Z={z:+.4f}G")
        print(f"heading = {mag.heading():.1f} deg")
"""

import math
import os
import time
import warnings

from .calibration import CalibrationError, MagCalibration, describe_spin
from .orientation import (
    Orientation,
    OrientationError,
    axis_letter,
    detect_up_axis,
    rotation_summary,
    up_axis_for,
)
from .report import print_calibration_report, print_rotation_report

try:
    from smbus2 import SMBus, i2c_msg
except ImportError as exc:
    raise ImportError(
        "This module requires smbus2. Install it with: pip install smbus2"
    ) from exc

# Magnetic declination in Czechia, 2026: magnetic north is about this far
# east of true north. Add it to convert a magnetic bearing to a true one.
DECLINATION_CZ_2026 = 5.5

DEFAULT_CALIBRATION_FILE = "qmc5883p-calibration.json"


class QMC5883PError(RuntimeError):
    """Raised when the sensor doesn't respond as expected."""


class QMC5883P:
    DEFAULT_ADDRESS = 0x2C

    # Registers
    REG_CHIP_ID = 0x00      # expected value: 0x80
    REG_DATA_START = 0x01   # 0x01..0x06: X_L, X_H, Y_L, Y_H, Z_L, Z_H
    REG_STATUS = 0x09       # bit0 = DRDY (new data ready)
    REG_CTRL1 = 0x0A        # mode + output data rate
    REG_CTRL2 = 0x0B        # field range
    REG_MYSTERY_0D = 0x0D   # part of QST's recommended init sequence
    REG_SIGN_0x29 = 0x29    # part of QST's recommended init sequence

    CHIP_ID_VALUE = 0x80

    # Delay between the "write register pointer" transaction and the
    # "read data" transaction. Works around MCP2221 firmware/driver
    # quirks around combined/repeated-start transactions.
    INTER_TRANSACTION_DELAY = 0.003
    READ_RETRIES = 2

    MODE_SUSPEND = 0b00
    MODE_CONTINUOUS = 0b11

    _ODR_BITS = {10: 0b00, 50: 0b01, 100: 0b10, 200: 0b11}

    # range -> (CTRL2 RNG bits, LSB-per-Gauss sensitivity)
    _RANGE_TABLE = {
        "2G": (0b11, 15000.0),
        "8G": (0b10, 3750.0),
        "12G": (0b01, 2500.0),
        "30G": (0b00, 1000.0),
    }

    def __init__(self, bus=1, address=DEFAULT_ADDRESS, odr=100, field_range="8G",
                 orientation=None, calibration=None):
        """
        orientation: an Orientation describing how the board is mounted.
                     Defaults to flat and chip-side up, with +X forward.
        calibration: a MagCalibration for this assembly. Defaults to
                     identity, which is only correct if nothing magnetic
                     turns with the sensor -- in practice, capture one
                     with calibrate_spin() or the --calibrate CLI mode.
        """
        if odr not in self._ODR_BITS:
            raise ValueError(f"odr must be one of {sorted(self._ODR_BITS)}")
        if field_range not in self._RANGE_TABLE:
            raise ValueError(f"field_range must be one of {sorted(self._RANGE_TABLE)}")

        self.address = address
        self.odr = odr
        self.field_range = field_range
        self._sensitivity = self._RANGE_TABLE[field_range][1]
        self.orientation = orientation or Orientation()
        self.calibration = calibration or MagCalibration()
        self._warn_on_range_mismatch()

        self._bus = SMBus(bus)
        self._init_sensor()

    def _warn_on_range_mismatch(self):
        recorded = self.calibration.field_range
        if recorded is not None and recorded != self.field_range:
            warnings.warn(
                f"calibration was captured at range {recorded} but the sensor is "
                f"set to {self.field_range}; hard-iron offsets are in raw LSB and "
                "do not carry across ranges. Recalibrate, or use the same range.",
                stacklevel=3,
            )

    # -- context manager -------------------------------------------------
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def close(self):
        try:
            self.set_mode_suspend()
        except OSError as exc:
            # Suspending is a courtesy, not a requirement. If the bridge has
            # already wedged, letting this propagate from __exit__ would mask
            # whatever actually went wrong inside the with-block.
            warnings.warn(f"could not suspend the sensor while closing: {exc}", stacklevel=2)
        finally:
            self._bus.close()

    # -- low-level I2C helpers ------------------------------------------
    def _read_bytes(self, register, length):
        """
        Read `length` bytes starting at `register` using two fully
        separate I2C transactions (write-pointer, then read), rather
        than one combined repeated-start transaction. This avoids
        relying on repeated-start support, which some USB-I2C bridges
        (notably the MCP2221) handle unreliably.
        """
        last_exc = None
        for attempt in range(self.READ_RETRIES + 1):
            try:
                write = i2c_msg.write(self.address, [register])
                self._bus.i2c_rdwr(write)
                if self.INTER_TRANSACTION_DELAY:
                    time.sleep(self.INTER_TRANSACTION_DELAY)
                read = i2c_msg.read(self.address, length)
                self._bus.i2c_rdwr(read)
                return list(read)
            except OSError as exc:
                last_exc = exc
                time.sleep(0.05 * (attempt + 1))
        raise QMC5883PError(
            f"I2C read of register 0x{register:02X} failed after "
            f"{self.READ_RETRIES + 1} attempts (last error: {last_exc}). "
            "If using an MCP2221, try unplugging and replugging it -- "
            "a failed read can leave its internal state machine stuck."
        ) from last_exc

    def _read_byte(self, register):
        return self._read_bytes(register, 1)[0]

    # -- setup -------------------------------------------------------------
    def _init_sensor(self):
        chip_id = self._read_byte(self.REG_CHIP_ID)
        if chip_id != self.CHIP_ID_VALUE:
            raise QMC5883PError(
                f"Unexpected chip ID 0x{chip_id:02X} (expected 0x{self.CHIP_ID_VALUE:02X}). "
                "Wrong address, wrong chip, or wiring issue."
            )

        # Registers 0x0D and 0x29 aren't documented in the public English
        # datasheet excerpts, but appear in QST's own reference init
        # sequence for the QMC5883P. Writing them mirrors that sequence;
        # the sensor also works without them in basic testing, but leave
        # them in for closer-to-spec behavior.
        self._bus.write_byte_data(self.address, self.REG_MYSTERY_0D, 0x40)
        self._bus.write_byte_data(self.address, self.REG_SIGN_0x29, 0x06)

        self.set_range(self.field_range)
        self.set_mode_continuous(self.odr)

    def set_range(self, field_range):
        if field_range not in self._RANGE_TABLE:
            raise ValueError(f"field_range must be one of {sorted(self._RANGE_TABLE)}")
        rng_bits, sensitivity = self._RANGE_TABLE[field_range]
        self.field_range = field_range
        self._sensitivity = sensitivity
        value = (rng_bits & 0b11) << 2
        self._bus.write_byte_data(self.address, self.REG_CTRL2, value)
        self._warn_on_range_mismatch()

    def set_mode_continuous(self, odr=100):
        if odr not in self._ODR_BITS:
            raise ValueError(f"odr must be one of {sorted(self._ODR_BITS)}")
        self.odr = odr
        odr_bits = self._ODR_BITS[odr]
        # Upper nibble (0xC0) matches QST's reference design; lower
        # nibble packs ODR (bits 3:2) and MODE (bits 1:0).
        value = 0xC0 | (odr_bits << 2) | self.MODE_CONTINUOUS
        self._bus.write_byte_data(self.address, self.REG_CTRL1, value)

    def set_mode_suspend(self):
        value = 0xC0 | self.MODE_SUSPEND
        self._bus.write_byte_data(self.address, self.REG_CTRL1, value)

    # -- reading -------------------------------------------------------------
    def data_ready(self):
        status = self._read_byte(self.REG_STATUS)
        return bool(status & 0x01)

    def wait_for_data(self, timeout=1.0):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.data_ready():
                return True
            time.sleep(0.001)
        return False

    @staticmethod
    def _to_signed16(low_byte, high_byte):
        value = (high_byte << 8) | low_byte
        if value >= 0x8000:
            value -= 0x10000
        return value

    def read_raw(self, wait=True, timeout=1.0):
        """
        Return raw signed 16-bit (x, y, z) readings, exactly as the chip
        reports them -- no calibration, no orientation. This is what
        calibration is derived from.
        """
        if wait:
            self.wait_for_data(timeout=timeout)
        data = self._read_bytes(self.REG_DATA_START, 6)
        return (
            self._to_signed16(data[0], data[1]),
            self._to_signed16(data[2], data[3]),
            self._to_signed16(data[4], data[5]),
        )

    def read_corrected(self, wait=True, timeout=1.0):
        """Return calibrated (x, y, z) still in raw LSB units."""
        return self.calibration.apply(self.read_raw(wait=wait, timeout=timeout))

    def read_gauss(self, wait=True, timeout=1.0):
        """Return calibrated (x, y, z) scaled to Gauss for the current range."""
        x, y, z = self.read_corrected(wait=wait, timeout=timeout)
        s = self._sensitivity
        return x / s, y / s, z / s

    def heading(self, declination_deg=0.0, samples=1):
        """
        Compass bearing of the mount's `forward` axis, in degrees
        [0, 360), assuming the orientation's forward/left plane is
        horizontal. Positive declination corrects magnetic north to true
        north (see DECLINATION_CZ_2026).

        `samples` averages several readings as unit vectors, which
        smooths noise without the wraparound bug you get from averaging
        the angles themselves.
        """
        if samples < 1:
            raise ValueError("samples must be at least 1")
        east = north = 0.0
        for _ in range(samples):
            forward, left = self.orientation.horizontal(self.read_corrected())
            magnitude = math.hypot(forward, left)
            if magnitude == 0.0:
                continue
            north += forward / magnitude
            east += left / magnitude
        if north == 0.0 and east == 0.0:
            raise QMC5883PError("no horizontal field component -- cannot compute a heading")
        return (math.degrees(math.atan2(east, north)) + declination_deg) % 360.0

    def field_strength(self):
        """Total calibrated field magnitude in Gauss (Earth's is ~0.25-0.65 G)."""
        return math.hypot(*self.read_gauss())

    # -- calibration -------------------------------------------------------
    def capture_spin(self, seconds=30.0, interval=0.02, progress=None):
        """
        Collect raw samples for `seconds` while you rotate the sensor.
        `progress` is called with (elapsed, seconds) as it goes.

        A read failure ends the capture early and returns what was
        collected so far, rather than discarding it -- a wedged MCP2221
        cannot be recovered by retrying anyway, and a partial spin may
        still be enough to calibrate from. Check the sample count and
        describe_spin() coverage before trusting a truncated capture.
        """
        samples = []
        start = time.monotonic()
        last_report = -1.0
        while True:
            elapsed = time.monotonic() - start
            if elapsed >= seconds:
                break
            try:
                samples.append(self.read_raw())
            except QMC5883PError as exc:
                warnings.warn(
                    f"capture stopped after {elapsed:.1f}s / {len(samples)} samples: {exc}",
                    stacklevel=2,
                )
                break
            if progress is not None and elapsed - last_report >= 1.0:
                progress(elapsed, seconds)
                last_report = elapsed
            time.sleep(interval)
        return samples

    def calibrate_spin(self, seconds=30.0, interval=0.02, progress=None):
        """
        Capture a rotation, derive a calibration from it, install it on
        this instance, and return (calibration, report).

        Keep the sensor level and turn it slowly through at least one
        full revolution -- two is better. `report` comes from
        describe_spin(); check its "full_turn" flag before trusting the
        result.
        """
        return self._calibrate_from_capture(seconds, interval, progress)

    def calibrate_tumble(self, seconds=60.0, interval=0.02, progress=None):
        """
        Same as calibrate_spin(), but for a capture where the sensor is
        turned through *every* orientation -- rolled and pitched as well
        as spun, tracing a sphere rather than a circle.

        This is the only way to get a usable offset for the vertical
        axis, and therefore the only calibration detect_up() will accept.
        Sixty seconds of unhurried tumbling in all directions is plenty.
        """
        return self._calibrate_from_capture(seconds, interval, progress)

    def _calibrate_from_capture(self, seconds, interval, progress):
        samples = self.capture_spin(seconds=seconds, interval=interval, progress=progress)
        report = describe_spin(samples)
        # from_samples works out for itself which axes were swept, so the
        # spin and tumble paths differ only in what you do with your hands.
        self.calibration = MagCalibration.from_samples(samples, field_range=self.field_range)
        return self.calibration, report

    def load_calibration(self, path=DEFAULT_CALIBRATION_FILE, required=False):
        """
        Install a calibration from a JSON file. Returns True if one was
        loaded, False if the file is absent and `required` is False.
        """
        if not os.path.exists(path):
            if required:
                raise CalibrationError(f"no calibration file at {path}")
            return False
        self.calibration = MagCalibration.load(path)
        self._warn_on_range_mismatch()
        return True

    def check_rotation(self, seconds=20.0, interval=0.05, progress=None):
        """
        Record headings while the sensor is turned, and summarise the
        result. Turn it clockwise as seen from above: the summary's
        `direction` should read "clockwise" and `full_turn` be True.

        See orientation.rotation_summary() for how to read the verdict.
        """
        headings = []
        start = time.monotonic()
        last_report = -1.0
        while True:
            elapsed = time.monotonic() - start
            if elapsed >= seconds:
                break
            try:
                headings.append(self.heading())
            except (QMC5883PError, CalibrationError) as exc:
                warnings.warn(f"check stopped after {elapsed:.1f}s: {exc}", stacklevel=2)
                break
            if progress is not None and elapsed - last_report >= 1.0:
                progress(elapsed, seconds)
                last_report = elapsed
            time.sleep(interval)
        return rotation_summary(headings), headings

    def detect_up(self, axis=None, samples=16):
        """
        Work out the mount's `up` axis from the current field. Hold the
        board level in its intended orientation first.

        Pass `axis` ('z') when you already know which axis is vertical --
        a level spin tells you, as the axis it does not sweep -- and only
        its sign is determined, which needs no assumption about the local
        dip angle. With `axis` left as None the vertical axis is guessed
        by size, which fails where local iron flattens the apparent dip.

        Either way this requires a calibration that swept all three axes
        (see calibrate_tumble()). A level spin leaves the vertical axis's
        offset indistinguishable from Earth's vertical field, and
        subtracting it zeroes exactly the component needed here.
        """
        if not self.calibration.covers_all_axes:
            raise CalibrationError(
                "cannot detect the vertical axis from a calibration that did not "
                f"sweep {', '.join(self.calibration.unswept_axes)}: that axis's "
                "offset absorbed Earth's vertical field, so the calibrated reading "
                "for it is ~0 by construction. Run a tumble calibration "
                "(--calibrate --tumble) instead of a level spin."
            )
        totals = [0.0, 0.0, 0.0]
        for _ in range(samples):
            reading = self.read_corrected()
            totals = [t + v for t, v in zip(totals, reading)]
        mean = tuple(t / samples for t in totals)
        if axis is None:
            return detect_up_axis(mean)
        return up_axis_for(axis, mean["xyz".index(axis_letter(axis))])


# -- CLI -------------------------------------------------------------------
def _build_parser():
    import argparse

    parser = argparse.ArgumentParser(
        description="Read, calibrate and orient the QMC5883P magnetometer",
    )
    parser.add_argument("--bus", type=int, default=1, help="I2C bus number (see: ls /dev/i2c-*)")
    parser.add_argument("--address", type=lambda v: int(v, 0), default=QMC5883P.DEFAULT_ADDRESS)
    parser.add_argument("--range", dest="field_range", default="8G",
                        choices=sorted(QMC5883P._RANGE_TABLE), help="field range")
    # Note the "=" in the examples: argparse reads a bare "-z" as a flag.
    parser.add_argument("--up", default="+z",
                        help="sensor axis pointing away from the ground; "
                             "write it as --up=-z, not --up -z")
    parser.add_argument("--forward", default="+x",
                        help="sensor axis whose bearing is reported; "
                             "write it as --forward=-x, not --forward -x")
    parser.add_argument("--declination", type=float, default=0.0,
                        help=f"magnetic declination in degrees "
                             f"(Czechia 2026: {DECLINATION_CZ_2026})")
    parser.add_argument("--cal", default=DEFAULT_CALIBRATION_FILE,
                        help="calibration JSON file to load and/or write")
    parser.add_argument("--no-cal", action="store_true",
                        help="ignore the calibration file and report uncorrected values")
    parser.add_argument("--rate", type=float, default=2.0, help="prints per second")

    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--calibrate", action="store_true",
                      help="capture a rotation and write a calibration file")
    mode.add_argument("--check-rotation", action="store_true",
                      help="turn the sensor clockwise while this runs, to verify the "
                           "mount description end-to-end")
    mode.add_argument("--detect-up", action="store_true",
                      help="report which sensor axis is currently vertical; needs a "
                           "--tumble calibration and the board held level")
    parser.add_argument("--axis", default=None,
                        help="for --detect-up: the axis you already know is vertical "
                             "(e.g. z), so only its sign has to be determined. More "
                             "reliable than letting it guess by magnitude.")
    parser.add_argument("--tumble", action="store_true",
                        help="calibrate by turning the sensor through every "
                             "orientation, not just a level spin; required before "
                             "--detect-up works")
    parser.add_argument("--seconds", type=float, default=None,
                        help="capture duration for --calibrate "
                             "(default: 30 for a spin, 60 for a tumble)")
    return parser


def _run_calibrate(mag, args):
    seconds = args.seconds
    if args.tumble:
        seconds = 60.0 if seconds is None else seconds
        print(f"Turn the sensor through EVERY orientation -- spin it, roll it, stand "
              f"it on each edge, unhurriedly. Capturing for {seconds:.0f}s...")
    else:
        seconds = 30.0 if seconds is None else seconds
        print(f"Rotate the sensor SLOWLY through at least two full turns, keeping it "
              f"level. Capturing for {seconds:.0f}s...")

    def progress(elapsed, total):
        print(f"  {elapsed:5.1f}/{total:.0f}s", end="\r", flush=True)

    capture = mag.calibrate_tumble if args.tumble else mag.calibrate_spin
    cal, report = capture(seconds=seconds, progress=progress)
    print(" " * 24, end="\r")

    print_calibration_report(cal, report)

    cal.save(args.cal)
    print(f"Written to {args.cal}")


def _run_check_rotation(mag, args, orientation):
    seconds = 20.0 if args.seconds is None else args.seconds
    print(f"Turn the sensor CLOCKWISE (seen from above) through a full turn, "
          f"keeping it level. Recording for {seconds:.0f}s...")

    def progress(elapsed, total):
        print(f"  {elapsed:5.1f}/{total:.0f}s", end="\r", flush=True)

    report, headings = mag.check_rotation(seconds=seconds, progress=progress)
    print(" " * 24, end="\r")

    ok = print_rotation_report(report, headings, orientation)
    return 0 if ok else 1


def main(argv=None):
    args = _build_parser().parse_args(argv)
    orientation = Orientation(up=args.up, forward=args.forward)

    with QMC5883P(bus=args.bus, address=args.address, field_range=args.field_range,
                  orientation=orientation) as mag:
        if args.calibrate:
            _run_calibrate(mag, args)
            return 0

        if args.no_cal:
            print("Running WITHOUT calibration -- headings will be wrong if anything "
                  "magnetic turns with the sensor.")
        elif mag.load_calibration(args.cal):
            print(f"Loaded calibration from {args.cal}")
        else:
            print(f"No calibration at {args.cal} -- run with --calibrate first. "
                  "Headings below are probably meaningless.")

        if args.check_rotation:
            return _run_check_rotation(mag, args, orientation)

        if args.detect_up:
            try:
                print(f"Vertical axis appears to be: up={mag.detect_up(args.axis)!r} "
                      f"(pass it as --up, or Orientation(up=...))")
            except (CalibrationError, OrientationError) as exc:
                print(f"Cannot determine the vertical axis: {exc}")
                return 1
            return 0

        print(f"Chip at 0x{args.address:02X}, mount up={orientation.up} "
              f"forward={orientation.forward} left={orientation.left}. "
              "Streaming... Ctrl+C to stop.")
        try:
            while True:
                x, y, z = mag.read_gauss()
                print(f"X={x:+.4f}G  Y={y:+.4f}G  Z={z:+.4f}G  |B|={math.sqrt(x*x+y*y+z*z):.4f}G"
                      f"  heading={mag.heading(args.declination, samples=4):6.1f} deg")
                time.sleep(1.0 / args.rate)
        except KeyboardInterrupt:
            pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
