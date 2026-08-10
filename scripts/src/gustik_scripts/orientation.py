"""
Mounting orientation for a 3-axis magnetometer.

The sensor reports a field vector in its own axes, as printed on the chip
package. Turning that into a compass heading requires knowing how the
board is actually mounted -- which sensor axis points up, and which one
points along the direction whose bearing you want to report.

Rather than a pile of per-axis "invert this" booleans (which reverse the
direction the heading appears to rotate in, and are found by trial and
error), declare the mount geometry once:

    Orientation(up="-z", forward="+x")   # board flipped over, +X forward
    Orientation(up="+y", forward="+z")   # board standing on edge

Everything else -- including the handedness of the rotation -- follows.

Frame convention
    A right-handed body frame is built from the two axes you name:

        forward  the direction whose bearing is reported
        up       away from the ground
        left     up x forward  (completes the right-handed set)

    Heading is the compass bearing of `forward`: 0 deg at magnetic north,
    increasing clockwise when viewed from above, so 90 deg is east.

Tilt
    Heading uses only the forward/left plane, i.e. it assumes that plane
    is horizontal. A vertical mount is fully supported -- just name the
    axes accordingly -- but the *board* must stay upright. This chip has
    no accelerometer, so there is no way to compensate for heel or pitch
    from its readings alone; expect error to grow with tilt, roughly in
    proportion to the field's dip angle at your location.
"""

import math
from dataclasses import dataclass, field

__all__ = [
    "Orientation",
    "OrientationError",
    "detect_up_axis",
    "up_axis_for",
    "axis_letter",
    "rotation_summary",
    "AXES",
]


class OrientationError(ValueError):
    """Raised for a mount description that cannot be interpreted."""


AXES = {
    "+x": (1.0, 0.0, 0.0),
    "-x": (-1.0, 0.0, 0.0),
    "+y": (0.0, 1.0, 0.0),
    "-y": (0.0, -1.0, 0.0),
    "+z": (0.0, 0.0, 1.0),
    "-z": (0.0, 0.0, -1.0),
}


def _normalise_axis(spec):
    """Accept '+z', '-z', 'z', 'Z' ... and return the canonical '+z' form."""
    if not isinstance(spec, str):
        raise OrientationError(f"axis must be a string like '+z', got {spec!r}")
    text = spec.strip().lower()
    if text in ("x", "y", "z"):
        text = "+" + text
    if text not in AXES:
        raise OrientationError(
            f"unknown axis {spec!r}; expected one of {sorted(AXES)} (or a bare x/y/z)"
        )
    return text


def axis_letter(spec):
    """Return just the axis letter of '+z' / '-z' / 'z' -- always 'z'."""
    return _normalise_axis(spec)[1]


def _cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


@dataclass(frozen=True)
class Orientation:
    """
    How the sensor is mounted, as two sensor-frame axis names.

    up:      the sensor axis pointing away from the ground
    forward: the sensor axis pointing along the reference direction
             whose bearing heading() reports
    """

    up: str = "+z"
    forward: str = "+x"

    _up: tuple = field(init=False, repr=False, compare=False)
    _forward: tuple = field(init=False, repr=False, compare=False)
    _left: tuple = field(init=False, repr=False, compare=False)

    def __post_init__(self):
        up = _normalise_axis(self.up)
        forward = _normalise_axis(self.forward)
        if up[1] == forward[1]:
            raise OrientationError(
                f"up ({up}) and forward ({forward}) must be different axes -- "
                "they describe two perpendicular directions"
            )
        object.__setattr__(self, "up", up)
        object.__setattr__(self, "forward", forward)
        object.__setattr__(self, "_up", AXES[up])
        object.__setattr__(self, "_forward", AXES[forward])
        # left = up x forward completes a right-handed (forward, left, up) set.
        object.__setattr__(self, "_left", _cross(AXES[up], AXES[forward]))

    @property
    def left(self):
        """Canonical name of the derived third axis, e.g. '-y'."""
        for name, vector in AXES.items():
            if vector == self._left:
                return name
        raise AssertionError("derived left axis is not an axis")  # unreachable

    def horizontal(self, vector):
        """Return the (forward, left) components of a sensor-frame vector."""
        return _dot(vector, self._forward), _dot(vector, self._left)

    def vertical(self, vector):
        """Return the component of a sensor-frame vector along `up`."""
        return _dot(vector, self._up)

    def heading(self, vector, declination_deg=0.0):
        """
        Compass bearing of the `forward` direction, in degrees [0, 360).

        `vector` is a calibrated sensor-frame field reading. Positive
        declination corrects magnetic north to true north (about +5.5 deg
        in Czechia in 2026).
        """
        forward_component, left_component = self.horizontal(vector)
        bearing = math.degrees(math.atan2(left_component, forward_component))
        return (bearing + declination_deg) % 360.0


# How far a capture must have turned, in degrees, before its direction is
# reported rather than dismissed as a wobble.
MIN_SWEEP = 45.0


def rotation_summary(headings, sectors=12, minimum_sweep=MIN_SWEEP):
    """
    Summarise a sequence of headings recorded during one rotation, to
    check a mount description end-to-end.

    Sums the shortest step between consecutive headings, so a capture
    that passes through north is not mistaken for a leap backwards.

    Turn the board clockwise as seen from above: `direction` must come
    back "clockwise". If it says "counter-clockwise", the mount's `up`
    axis is inverted -- flip its sign -- because `left` is derived as
    up x forward, and a flipped `up` mirrors the plane.
    """
    if len(headings) < 2:
        raise OrientationError("need at least two headings to judge a rotation")

    sweep = 0.0
    for previous, current in zip(headings, headings[1:]):
        step = (current - previous) % 360.0
        if step > 180.0:
            step -= 360.0
        sweep += step

    if abs(sweep) < minimum_sweep:
        direction = "unclear"
    elif sweep > 0.0:
        direction = "clockwise"
    else:
        direction = "counter-clockwise"

    width = 360.0 / sectors
    visited = {int(h % 360.0 // width) for h in headings}
    return {
        "count": len(headings),
        "sweep": sweep,
        "direction": direction,
        "sectors": sectors,
        "sectors_covered": len(visited),
        "full_turn": len(visited) == sectors,
    }


MIN_VERTICAL_READING = 1.0


def up_axis_for(axis, reading, hemisphere="north", minimum=MIN_VERTICAL_READING):
    """
    Given which sensor axis is vertical, work out which way it points.

    `axis` names the vertical axis ('z', or '+z' -- the sign is ignored,
    since determining it is the whole point). `reading` is that axis's
    *calibrated* value with the sensor held level.

    Earth's field dips into the ground in the northern hemisphere, so a
    positive reading means the axis points down, and `up` is its
    negation. Only the sign of `reading` is used, which is what makes
    this reliable: it needs no assumption about the local dip angle, and
    works even where the vertical component is smaller than the
    horizontal one.

    Identify the axis first with a level spin -- it is the one that does
    not sweep. See calibration.describe_spin()'s "vertical_candidate".
    """
    if hemisphere not in ("north", "south"):
        raise OrientationError("hemisphere must be 'north' or 'south'")
    letter = _normalise_axis(axis)[1]

    if abs(reading) < minimum:
        raise OrientationError(
            f"the {letter} reading ({reading:+.1f}) is too close to zero for its sign "
            "to mean anything. Two usual causes: the calibration was captured from a "
            f"level spin, which zeroes {letter} by construction (run a tumble "
            "calibration instead), or the sensor is not actually level."
        )

    dips_positive = reading > 0.0
    if hemisphere == "south":
        dips_positive = not dips_positive
    # A positive reading on the vertical axis means that axis points down.
    return ("-" if dips_positive else "+") + letter


def detect_up_axis(vector, dominance=1.5, hemisphere="north"):
    """
    Guess both which sensor axis is vertical and which way it points,
    from a single calibrated reading taken with the board held level.

    This is a convenience shortcut, and only works where the field dips
    steeply enough that its vertical component is clearly the largest --
    nominally about 66 deg below horizontal in Czechia. Local soft iron
    can flatten the apparent dip enough to break that; this function
    then refuses rather than guessing. When it does, identify the
    vertical axis from a level spin and call up_axis_for() instead.

    `dominance` is how many times larger the vertical component must be
    than the next largest before the answer is trusted.
    """
    magnitudes = sorted(
        ((abs(v), i) for i, v in enumerate(vector)), key=lambda pair: pair[0], reverse=True
    )
    largest, index = magnitudes[0]
    runner_up = magnitudes[1][0]
    if largest == 0.0 or largest < dominance * runner_up:
        raise OrientationError(
            f"no axis dominates in {tuple(round(v, 1) for v in vector)}, so the "
            "vertical one cannot be picked out by size alone -- the sensor may not be "
            "level, or local iron has flattened the apparent dip angle. Identify the "
            "vertical axis from a level spin (it is the axis that does not sweep) and "
            "use up_axis_for() to get its sign."
        )
    return up_axis_for("xyz"[index], vector[index], hemisphere=hemisphere)
