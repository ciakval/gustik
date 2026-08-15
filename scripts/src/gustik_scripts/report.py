"""
report.py

Human-readable verdicts on a capture, shared by both front ends: the I2C
bench driver (qmc5883p.py, needs a USB-I2C bridge) and the ESP32 serial
calibrator (mag_calibrate.py, uses the real station hardware).

Kept in one place deliberately. This is the text that decides whether a
capture gets trusted, and two copies of it would eventually disagree about
what "good enough" means.
"""

import math

from .firmware_output import motion_warning

__all__ = ["print_calibration_report", "print_rotation_report"]


def print_calibration_report(calibration, report, out=print):
    """
    Describe a capture and whether it is fit to calibrate from.

    `report` is a describe_spin() result. Judges the hard-iron bias only in
    the plane of rotation: the vertical axis plays no part in heading, and a
    level spin cannot measure its offset anyway, so including it would
    overstate the problem.
    """
    out(
        f"{report['count']} samples, widest plane is "
        f"{report['plane'][0]}-{report['plane'][1]}"
    )
    for i, axis in enumerate("xyz"):
        swept = "swept" if i in calibration.swept else "NOT swept"
        out(
            f"  {axis}: span={report['span'][i]:8.1f}  centre={report['centre'][i]:+9.1f}  "
            f"scale={calibration.scale[i]:.4f}  ({swept})"
        )
    out(
        f"  coverage: {report['sectors_covered']}/{report['sectors']} sectors "
        f"of a turn in the {report['plane'][0]}-{report['plane'][1]} plane"
    )

    # Checked before the coverage verdict, because it invalidates it: a
    # stationary capture reports full coverage.
    stalled = motion_warning(report, calibration.field_range)
    if stalled:
        out(f"!! NOT A ROTATION: {stalled}")
    elif not report["full_turn"]:
        out(
            "WARNING: the turn was incomplete, so the centre is biased. "
            "Re-run and rotate further."
        )
    if calibration.covers_all_axes:
        out(
            "  all three axes were swept, so every offset is real "
            "and --detect-up will work"
        )
    elif report["vertical_candidate"]:
        out(
            f"  {report['vertical_candidate']} did not sweep, so it is the axis you "
            f"turned about -- i.e. the vertical one, if you held the board level. "
            f"Confirm its sign with: --tumble, then "
            f"--detect-up --axis {report['vertical_candidate']}"
        )
    else:
        out(
            f"  the {', '.join(calibration.unswept_axes)} offset is Earth's field plus "
            "hard iron with no way to separate them -- fine for heading, which ignores "
            "that axis, but --detect-up needs a tumble capture"
        )

    plane = ["xyz".index(axis) for axis in report["plane"]]
    radius = sum(report["span"][i] for i in plane) / 4.0
    bias = math.hypot(*(calibration.offset[i] for i in plane))
    if radius > 0.0:
        out(
            f"  in-plane hard-iron bias is {bias / radius:.2f}x the field circle's "
            f"radius ({bias:.0f} vs {radius:.0f} LSB)"
        )
        if bias > radius:
            out(
                "  (that is why uncorrected headings were stuck in a narrow band: "
                "the origin sat outside the circle)"
            )


def print_rotation_report(report, headings, orientation, out=print):
    """
    Verdict on a --check-rotation capture. Returns True if the mount
    description is confirmed end-to-end.
    """
    out(
        f"{report['count']} headings, swept {report['sweep']:+.0f} deg total, "
        f"covering {report['sectors_covered']}/{report['sectors']} sectors "
        f"({min(headings):.0f}..{max(headings):.0f} deg)"
    )

    ok = True
    if not report["full_turn"]:
        out(
            f"INCOMPLETE: only {report['sectors_covered']}/{report['sectors']} sectors "
            "were visited. Either the turn was partial -- re-run and go further -- or "
            "the heading is still being squeezed into an arc, which means the "
            "calibration is stale. Recalibrate."
        )
        ok = False
    if report["direction"] == "clockwise":
        out(
            f"Direction is correct: up={orientation.up} forward={orientation.forward} "
            "is the right way round."
        )
    elif report["direction"] == "counter-clockwise":
        flipped = ("-" if orientation.up.startswith("+") else "+") + orientation.up[1]
        out(
            f"MIRRORED: heading fell as you turned clockwise. The mount's up axis is "
            f"inverted -- use --up={flipped} instead of --up={orientation.up}."
        )
        ok = False
    else:
        out("UNCLEAR: the sensor barely moved. Re-run and turn it a full revolution.")
        ok = False
    return ok
