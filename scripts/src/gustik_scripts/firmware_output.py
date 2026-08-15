"""
firmware_output.py

Renders a MagCalibration into the two forms the station firmware accepts.

The sign trap this module exists to absorb
    The chip reports a right-handed (x, y, z). The station's real mount is
    up=-z, forward=+x - the board is upside down - so
    firmware/src/sense/magnetometer.cpp negates Y as it reads, and hands
    correct/wind_direction.cpp an already-flipped frame. That function then
    subtracts the calibration offsets from what it was handed, so the Y offset
    it needs is the negative of the one measured here.

    That flip has to happen exactly once, somewhere. Doing it here means the
    emitted lines are paste-ready and the firmware applies them verbatim; the
    alternative is a human re-deriving a sign at 1am on a boat. The JSON
    calibration file keeps the chip's own frame, unflipped, because
    orientation.py reasons in that frame.

Soft iron is not emitted. MagnetometerCalibration carries hard-iron X/Y only -
v1 deliberately accepts hard-iron-only accuracy, which is ample for 8-octant
resolution (NFR-6). The scale factors stay in the JSON for whoever needs them
later.
"""

__all__ = [
    "CONFIG_KEY_OFFSET_X",
    "CONFIG_KEY_OFFSET_Y",
    "FIRMWARE_FIELD_RANGE",
    "LSB_PER_GAUSS",
    "MIN_ROTATION_GAUSS",
    "config_lines",
    "cpp_constant",
    "firmware_hard_iron",
    "motion_warning",
    "range_warning",
]

CONFIG_KEY_OFFSET_X = "mag.offsetX"
CONFIG_KEY_OFFSET_Y = "mag.offsetY"

# sense/magnetometer.cpp's begin() writes CTRL2 for 8G and nothing changes it
# at runtime. Offsets are raw LSB, so a calibration taken at any other range
# is silently wrong once loaded.
FIRMWARE_FIELD_RANGE = "8G"

# QMC5883P sensitivity per field range, from qmc5883p.py's register table.
LSB_PER_GAUSS = {"2G": 15000.0, "8G": 3750.0, "12G": 2500.0, "30G": 1000.0}

# Peak-to-peak field a capture must sweep in its widest plane before it counts
# as a rotation rather than a sensor sitting still.
#
# This check exists because describe_spin() cannot tell the two apart on its
# own: it normalises each axis by its own span before counting sectors, so
# stationary noise scatters around the full circle and reports a confident
# "12/12 sectors covered, all three axes swept". A capture where nobody
# actually turned the board therefore looks perfect and yields offsets that
# are simply wherever the sensor happened to be pointing -- which produces a
# stable, confident, wrong heading with no error anywhere.
#
# Earth's horizontal component is ~0.20 G in Czechia, so a real level turn
# sweeps ~0.40 G peak-to-peak (1500 LSB at 8G). Measured stationary noise on
# this hardware is ~40 LSB (~0.011 G). 0.08 G sits an order of magnitude above
# the noise and well below any genuine rotation.
MIN_ROTATION_GAUSS = 0.08


def firmware_hard_iron(calibration):
    """
    Return (offsetX, offsetY) in the firmware's own coordinate frame.

    Y is negated relative to `calibration.offset` - see the module docstring.
    """
    x, y, _z = calibration.offset
    return (x, -y)


def range_warning(calibration, expected=FIRMWARE_FIELD_RANGE):
    """
    Return a warning string if this calibration cannot be used by the firmware
    as it stands, or None if it is fine.
    """
    recorded = calibration.field_range
    if recorded is None:
        return (
            f"this calibration does not record which field range it was taken at; "
            f"the firmware runs at {expected} and offsets are in raw LSB, so use it "
            f"only if you know the capture was made at {expected}"
        )
    if recorded != expected:
        return (
            f"this calibration was taken at range {recorded} but the firmware runs at "
            f"{expected}; hard-iron offsets are in raw LSB and do not carry across "
            f"ranges. Recapture at {expected}."
        )
    return None


def motion_warning(report, field_range=FIRMWARE_FIELD_RANGE):
    """
    Return a warning if the capture does not look like a real rotation, or
    None if it does.

    `report` is a describe_spin() result. Judged on the raw peak-to-peak span
    of the widest plane, which is the one thing stationary noise cannot fake:
    sector coverage can, because it is measured after normalising each axis by
    its own span.
    """
    lsb_per_gauss = LSB_PER_GAUSS.get(field_range)
    if lsb_per_gauss is None:
        return None
    minimum = MIN_ROTATION_GAUSS * lsb_per_gauss
    plane = ["xyz".index(axis) for axis in report["plane"]]
    widest = max(report["span"][i] for i in plane)
    if widest >= minimum:
        return None
    return (
        f"the widest plane only swept {widest:.0f} LSB "
        f"({widest / lsb_per_gauss:.3f} G) peak-to-peak, below the "
        f"{minimum:.0f} LSB ({MIN_ROTATION_GAUSS} G) a real rotation produces. "
        "This looks like a stationary sensor, not a turn -- the 'sectors covered' "
        "figure above cannot tell the difference, because it rescales each axis "
        "by its own span first. The offsets below are then just wherever the "
        "sensor was pointing, and would give a stable, confident, WRONG heading. "
        "Turn the board through a full rotation and recapture."
    )


def config_lines(calibration, comment=None):
    """
    Lines to paste into firmware/data/config.txt, then upload with
    `pio run -t uploadfs` (a filesystem upload, not a reflash).

    This is the route to prefer when the calibration is expected to change
    again - remounting the sensor then costs a 2-second filesystem upload
    instead of a rebuild and reflash.
    """
    offset_x, offset_y = firmware_hard_iron(calibration)
    lines = [
        "# Magnetometer hard-iron offsets, raw LSB at field range "
        f"{calibration.field_range or 'unknown'}.",
        "# Captured with gustik_scripts.mag_calibrate; offsetY is already in the",
        "# firmware's Y-negated mount frame, so paste these verbatim.",
        "# Omit both keys to fall back to the defaults compiled into main.cpp.",
    ]
    if comment:
        lines.append(f"# {comment}")
    lines.append(f"{CONFIG_KEY_OFFSET_X}={offset_x:.1f}")
    lines.append(f"{CONFIG_KEY_OFFSET_Y}={offset_y:.1f}")
    return lines


def cpp_constant(calibration, comment=None):
    """
    Lines to paste over kMagnetometerCalibration in firmware/src/main.cpp.

    The alternative to config_lines(): it needs a reflash, but it keeps the
    measured value in version control and under review.
    """
    offset_x, offset_y = firmware_hard_iron(calibration)
    lines = [
        f"// Hard-iron offsets, raw LSB at field range "
        f"{calibration.field_range or 'unknown'}, captured with",
        "// gustik_scripts.mag_calibrate. hardIronOffsetY is the negative of the",
        "// JSON's raw Y offset - magnetometer.cpp returns Y already sign-flipped",
        "// for the up=-z/forward=+x mount.",
    ]
    if comment:
        lines.append(f"// {comment}")
    lines.append(
        "constexpr MagnetometerCalibration kMagnetometerCalibration{"
        f".hardIronOffsetX = {offset_x:.1f}, .hardIronOffsetY = {offset_y:.1f}}};"
    )
    return lines
