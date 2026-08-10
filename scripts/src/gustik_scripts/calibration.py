"""
Hard-iron and soft-iron calibration for a 3-axis magnetometer.

Why this is not optional
    A magnetometer measures the *total* field at the chip, which is
    Earth's field plus whatever the board is bolted to: nearby steel,
    the traces on the PCB, a USB connector shell, the breadboard's
    spring clips. Anything magnetic that turns with the sensor adds a
    constant vector in the sensor frame -- the "hard-iron" offset.

    Rotating a level sensor through a full turn should trace a circle
    centred on the origin, so that atan2 sweeps all 360 degrees. A
    hard-iron offset moves that circle sideways. Once the offset exceeds
    the circle's radius, the origin falls *outside* the circle entirely
    and atan2 can only ever sweep a narrow arc -- the heading appears
    stuck within a band of a few tens of degrees no matter how far you
    turn the sensor. That is the classic symptom, and the fix is to
    subtract the measured centre.

    Ferrous material nearby also distorts the circle into an ellipse
    ("soft iron"), which is corrected by rescaling each axis to a common
    radius.

Validity
    A calibration describes one rigid assembly. Move the sensor to a
    different mount, or change what sits next to it, and it must be
    redone. Offsets are in raw LSB, so it is also specific to the field
    range the sensor was set to when it was captured.
"""

import json
import math
from dataclasses import dataclass, field

__all__ = ["MagCalibration", "CalibrationError", "describe_spin"]

MIN_SAMPLES = 8

# An axis is treated as "swept" by the rotation if its span is at least
# this fraction of the widest axis's span. A flat spin never moves the
# vertical axis, and its tiny span carries no sensitivity information.
SWEPT_FRACTION = 0.5


class CalibrationError(ValueError):
    """Raised for unusable calibration data."""


def _spans(samples):
    """Return per-axis (minimum, maximum, centre, half-span)."""
    columns = list(zip(*samples))
    out = []
    for values in columns:
        low, high = min(values), max(values)
        out.append((low, high, (high + low) / 2.0, (high - low) / 2.0))
    return out


def _widest_two(radii):
    """Indices of the two axes with the largest half-spans, widest first."""
    return [i for _, i in sorted(((r, i) for i, r in enumerate(radii)), reverse=True)][:2]


@dataclass(frozen=True)
class MagCalibration:
    """
    Per-axis correction applied to raw readings.

    offset: hard-iron bias in raw LSB, subtracted from each axis
    scale:  soft-iron gain, multiplied in after the offset
    field_range: the sensor range the offsets were measured at, if known
                 (offsets are in LSB, so they do not carry across ranges)
    swept:  indices of the axes the capture actually rotated through.
            An axis missing from this list has an untrustworthy offset:
            a level spin holds the vertical axis constant, so the
            "centre" found for it is Earth's vertical field plus the hard
            iron, with no way to tell the two apart. Subtracting it
            zeroes the vertical component instead of debiasing it, which
            is harmless for heading (heading ignores that axis) but makes
            the calibrated vertical reading meaningless.
    """

    offset: tuple = (0.0, 0.0, 0.0)
    scale: tuple = (1.0, 1.0, 1.0)
    field_range: str = None
    swept: tuple = (0, 1, 2)

    def __post_init__(self):
        for name in ("offset", "scale"):
            values = getattr(self, name)
            try:
                triple = tuple(float(v) for v in values)
            except (TypeError, ValueError) as exc:
                raise CalibrationError(f"{name} must be three numbers, got {values!r}") from exc
            if len(triple) != 3:
                raise CalibrationError(f"{name} must have exactly 3 elements, got {values!r}")
            object.__setattr__(self, name, triple)
        if any(s == 0.0 for s in self.scale):
            raise CalibrationError(f"scale must not contain zeros, got {self.scale!r}")
        try:
            swept = tuple(sorted(int(i) for i in self.swept))
        except (TypeError, ValueError) as exc:
            raise CalibrationError(f"swept must be axis indices, got {self.swept!r}") from exc
        if any(i not in (0, 1, 2) for i in swept):
            raise CalibrationError(f"swept indices must be 0, 1 or 2, got {self.swept!r}")
        object.__setattr__(self, "swept", swept)

    @property
    def covers_all_axes(self):
        """True when every axis was rotated through, so all offsets are real."""
        return self.swept == (0, 1, 2)

    @property
    def unswept_axes(self):
        """Names of the axes whose offset should not be trusted."""
        return tuple("xyz"[i] for i in (0, 1, 2) if i not in self.swept)

    def apply(self, vector):
        """Correct a raw sensor-frame reading."""
        return tuple(
            (v - o) * s for v, o, s in zip(vector, self.offset, self.scale)
        )

    @classmethod
    def from_samples(cls, samples, field_range=None):
        """
        Derive a calibration from raw samples taken during a full, level
        rotation of the sensor about its vertical axis.

        Only the two axes actually swept by the rotation get a soft-iron
        scale; the third keeps a gain of 1.0, because a level spin gives
        no information about its sensitivity. Its offset is still
        recorded, but a level spin cannot measure it properly either --
        treat the vertical offset as a rough figure only.
        """
        samples = [tuple(s) for s in samples]
        if len(samples) < MIN_SAMPLES:
            raise CalibrationError(
                f"need at least {MIN_SAMPLES} samples to calibrate, got {len(samples)}"
            )
        if any(len(s) != 3 for s in samples):
            raise CalibrationError("every sample must be a 3-element (x, y, z) vector")

        stats = _spans(samples)
        offset = tuple(centre for _, _, centre, _ in stats)
        radii = [radius for _, _, _, radius in stats]

        widest = max(radii)
        if widest <= 0.0:
            raise CalibrationError(
                "the samples do not vary at all -- the sensor was not rotated, "
                "or the readings are stuck"
            )

        swept = [i for i, r in enumerate(radii) if r >= SWEPT_FRACTION * widest]
        if len(swept) < 2:
            raise CalibrationError(
                "only one axis varied, so no rotation plane can be identified -- "
                "rotate the sensor a full turn while keeping it level"
            )

        mean_radius = sum(radii[i] for i in swept) / len(swept)
        scale = tuple(
            mean_radius / radii[i] if i in swept else 1.0 for i in range(3)
        )
        return cls(
            offset=offset, scale=scale, field_range=field_range, swept=tuple(swept)
        )

    # -- persistence -----------------------------------------------------
    def to_dict(self):
        return {
            "offset": list(self.offset),
            "scale": list(self.scale),
            "field_range": self.field_range,
            "swept": list(self.swept),
        }

    @classmethod
    def from_dict(cls, data):
        if not isinstance(data, dict):
            raise CalibrationError(f"expected a JSON object, got {type(data).__name__}")
        missing = {"offset", "scale"} - set(data)
        if missing:
            raise CalibrationError(f"calibration is missing {sorted(missing)}")
        return cls(
            offset=data["offset"],
            scale=data["scale"],
            field_range=data.get("field_range"),
            swept=tuple(data.get("swept", (0, 1, 2))),
        )

    def save(self, path):
        with open(path, "w") as handle:
            json.dump(self.to_dict(), handle, indent=2, sort_keys=True)
            handle.write("\n")

    @classmethod
    def load(cls, path):
        try:
            with open(path) as handle:
                data = json.load(handle)
        except json.JSONDecodeError as exc:
            raise CalibrationError(f"{path} is not valid JSON: {exc}") from exc
        return cls.from_dict(data)


def describe_spin(samples, sectors=12):
    """
    Summarise a capture, to judge whether it is fit to calibrate from.

    Returns per-axis spans plus how much of a full turn the rotation
    plane actually covered, after removing the hard-iron offset. A
    capture that misses sectors will bias the derived centre.

    "vertical_candidate" names the axis a level spin left alone, which is
    by definition the one it turned about -- the vertical axis, if the
    board was held level. It is None when all three axes were swept
    (a tumble), because then nothing was held still. This identifies the
    vertical axis without assuming anything about the local dip angle;
    pair it with orientation.up_axis_for() to get its sign.
    """
    samples = [tuple(s) for s in samples]
    if not samples:
        raise CalibrationError("no samples")

    stats = _spans(samples)
    radii = [radius for _, _, _, radius in stats]
    first, second = _widest_two(radii)

    widest = max(radii)
    unswept = [
        i for i, r in enumerate(radii) if widest > 0.0 and r < SWEPT_FRACTION * widest
    ]
    vertical_candidate = "xyz"[unswept[0]] if len(unswept) == 1 else None

    visited = set()
    if radii[first] > 0.0 and radii[second] > 0.0:
        for sample in samples:
            a = (sample[first] - stats[first][2]) / radii[first]
            b = (sample[second] - stats[second][2]) / radii[second]
            angle = math.degrees(math.atan2(b, a)) % 360.0
            visited.add(int(angle // (360.0 / sectors)))

    return {
        "count": len(samples),
        "min": [low for low, _, _, _ in stats],
        "max": [high for _, high, _, _ in stats],
        "centre": [centre for _, _, centre, _ in stats],
        "span": [2.0 * radius for radius in radii],
        "plane": ("xyz"[first], "xyz"[second]),
        "vertical_candidate": vertical_candidate,
        "sectors": sectors,
        "sectors_covered": len(visited),
        "full_turn": len(visited) == sectors,
    }
