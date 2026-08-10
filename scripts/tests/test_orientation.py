"""Tests for the mounting-orientation model (pure math, no hardware)."""

import math
import unittest

from gustik_scripts.orientation import Orientation, OrientationError


def _field(bearing_deg, inclination_deg=66.0, strength=1.0):
    """
    Earth's field expressed in a world frame where X=north, Y=west, Z=up,
    as seen by a body whose *forward* direction points at `bearing_deg`.

    Returns the field in that body frame (forward, left, up).
    """
    # In the body frame, magnetic north sits at CCW angle `bearing` from forward.
    a = math.radians(bearing_deg)
    horizontal = strength * math.cos(math.radians(inclination_deg))
    # Positive inclination dips *into* the ground, so the up-component is negative.
    up = -strength * math.sin(math.radians(inclination_deg))
    return horizontal * math.cos(a), horizontal * math.sin(a), up


class TestAxisSpec(unittest.TestCase):
    def test_rejects_unknown_axis(self):
        with self.assertRaises(OrientationError):
            Orientation(up="+q", forward="+x")

    def test_rejects_parallel_up_and_forward(self):
        with self.assertRaises(OrientationError):
            Orientation(up="+z", forward="-z")

    def test_accepts_bare_axis_names(self):
        self.assertEqual(Orientation(up="z", forward="x"), Orientation(up="+z", forward="+x"))


class TestHeadingFlatMount(unittest.TestCase):
    """Sensor lying flat, chip facing up, +X pointing forward: the textbook case."""

    orientation = Orientation(up="+z", forward="+x")

    def test_matches_plain_atan2_of_y_over_x(self):
        # The classic QMC formula for a flat, upright board.
        for x, y in ((1.0, 0.0), (0.0, 1.0), (-1.0, 0.0), (0.7, -0.7)):
            expected = math.degrees(math.atan2(y, x)) % 360.0
            self.assertAlmostEqual(self.orientation.heading((x, y, 0.3)), expected, places=6)

    def test_recovers_the_bearing_it_was_given(self):
        for bearing in (0, 45, 90, 180, 270, 359):
            fwd, left, up = _field(bearing)
            # Flat + upright mount: forward=+X, left=+Y, up=+Z.
            self.assertAlmostEqual(
                self.orientation.heading((fwd, left, up)), bearing, places=6
            )

    def test_clockwise_rotation_increases_heading(self):
        # Turning the board clockwise (seen from above) must raise the heading.
        h1 = self.orientation.heading(_field(10))
        h2 = self.orientation.heading(_field(40))
        self.assertGreater(h2, h1)


class TestHeadingUpsideDown(unittest.TestCase):
    """Board flipped over about its X axis: +Z now points down, +Y points right."""

    orientation = Orientation(up="-z", forward="+x")

    def test_recovers_bearing_when_axes_are_flipped(self):
        for bearing in (0, 45, 90, 180, 270):
            fwd, left, up = _field(bearing)
            # Flipping about X keeps X, and negates Y and Z in the sensor frame.
            sensor = (fwd, -left, -up)
            self.assertAlmostEqual(self.orientation.heading(sensor), bearing, places=6)

    def test_differs_from_the_upright_reading(self):
        # A naive atan2(y, x) on an upside-down board mirrors the heading.
        fwd, left, up = _field(30)
        sensor = (fwd, -left, -up)
        naive = math.degrees(math.atan2(sensor[1], sensor[0])) % 360.0
        self.assertAlmostEqual(self.orientation.heading(sensor), 30.0, places=6)
        self.assertAlmostEqual(naive, 330.0, places=6)


class TestHeadingVerticalMount(unittest.TestCase):
    """Board standing on edge: +Y points up, +Z points forward."""

    orientation = Orientation(up="+y", forward="+z")

    def test_recovers_bearing_for_a_vertical_board(self):
        for bearing in (0, 45, 90, 180, 270):
            fwd, left, up = _field(bearing)
            # up=+Y, forward=+Z  =>  left = up x forward = +Y x +Z = +X.
            sensor = (left, up, fwd)
            self.assertAlmostEqual(self.orientation.heading(sensor), bearing, places=6)


class TestDeclination(unittest.TestCase):
    def test_declination_is_added(self):
        o = Orientation(up="+z", forward="+x")
        base = o.heading(_field(10))
        self.assertAlmostEqual(o.heading(_field(10), declination_deg=5.0), base + 5.0, places=6)

    def test_declination_wraps(self):
        o = Orientation(up="+z", forward="+x")
        self.assertAlmostEqual(o.heading(_field(358), declination_deg=5.0), 3.0, places=6)


class TestVerticalComponent(unittest.TestCase):
    def test_reports_component_along_the_up_axis(self):
        o = Orientation(up="-z", forward="+x")
        self.assertAlmostEqual(o.vertical((0.0, 0.0, -0.9)), 0.9, places=6)

    def test_northern_hemisphere_field_dips_downward(self):
        # Sanity check on the sign convention used by detect_up_axis().
        o = Orientation(up="+z", forward="+x")
        self.assertLess(o.vertical(_field(0)), 0.0)


class TestUpAxisSign(unittest.TestCase):
    """
    Given WHICH axis is vertical, decide which way it points. Uses only
    the sign of that axis's reading, so it holds at any dip angle.
    """

    def test_positive_reading_means_the_axis_points_down(self):
        from gustik_scripts.orientation import up_axis_for

        # Northern hemisphere: the field dips into the ground.
        self.assertEqual(up_axis_for("z", 1600.0), "-z")
        self.assertEqual(up_axis_for("z", -1600.0), "+z")

    def test_works_at_a_shallow_dip_where_the_axis_does_not_dominate(self):
        from gustik_scripts.orientation import up_axis_for

        # 979 LSB vertical against 1144 horizontal -- as measured on this rig.
        self.assertEqual(up_axis_for("z", 979.0), "-z")

    def test_southern_hemisphere_reverses_it(self):
        from gustik_scripts.orientation import up_axis_for

        self.assertEqual(up_axis_for("z", 1600.0, hemisphere="south"), "+z")

    def test_accepts_a_signed_axis_name(self):
        from gustik_scripts.orientation import up_axis_for

        self.assertEqual(up_axis_for("+y", 1600.0), "-y")

    def test_rejects_an_unknown_axis(self):
        from gustik_scripts.orientation import up_axis_for

        with self.assertRaises(OrientationError):
            up_axis_for("q", 1600.0)

    def test_refuses_a_reading_too_small_to_have_a_trustworthy_sign(self):
        from gustik_scripts.orientation import up_axis_for

        with self.assertRaises(OrientationError):
            up_axis_for("z", 0.0)


class TestDetectUpAxis(unittest.TestCase):
    """The convenience path: works only when the vertical axis dominates."""

    def test_finds_the_flipped_axis_from_a_northern_hemisphere_reading(self):
        from gustik_scripts.orientation import detect_up_axis

        # +Z pointing down: field dips into the ground, so sensor Z reads positive.
        self.assertEqual(detect_up_axis((200.0, -300.0, 1600.0)), "-z")
        # +Z pointing up: sensor Z reads negative.
        self.assertEqual(detect_up_axis((200.0, -300.0, -1600.0)), "+z")

    def test_finds_a_vertically_mounted_axis(self):
        from gustik_scripts.orientation import detect_up_axis

        self.assertEqual(detect_up_axis((100.0, 1700.0, -200.0)), "-y")

    def test_refuses_when_no_axis_dominates(self):
        from gustik_scripts.orientation import detect_up_axis

        with self.assertRaises(OrientationError):
            detect_up_axis((500.0, 500.0, 500.0))

    def test_refuses_at_a_shallow_dip_rather_than_guessing(self):
        from gustik_scripts.orientation import detect_up_axis

        # The real measurement from this rig: vertical is not the largest.
        with self.assertRaises(OrientationError):
            detect_up_axis((800.0, -820.0, 979.0))


class TestRotationSummary(unittest.TestCase):
    """
    Verifying a mount end-to-end: turn the board clockwise and the
    reported heading must rise through a full turn. Catches a mirrored
    mount, which is otherwise invisible in a static reading.
    """

    def setUp(self):
        from gustik_scripts.orientation import rotation_summary

        self.summarise = rotation_summary

    def test_a_clockwise_turn_sweeps_positive(self):
        report = self.summarise([i * 10.0 for i in range(36)])
        self.assertAlmostEqual(report["sweep"], 350.0, places=6)
        self.assertEqual(report["direction"], "clockwise")

    def test_a_counter_clockwise_turn_sweeps_negative(self):
        report = self.summarise([(-i * 10.0) % 360.0 for i in range(36)])
        self.assertAlmostEqual(report["sweep"], -350.0, places=6)
        self.assertEqual(report["direction"], "counter-clockwise")

    def test_wrapping_past_zero_is_not_counted_as_a_jump_backwards(self):
        # Naive differencing would see 355 -> 0 as -355 and call this a
        # large counter-clockwise turn.
        report = self.summarise([350.0, 355.0, 0.0, 5.0, 10.0], minimum_sweep=10.0)
        self.assertAlmostEqual(report["sweep"], 20.0, places=6)
        self.assertEqual(report["direction"], "clockwise")

    def test_a_short_turn_is_reported_as_unclear(self):
        # 20 degrees is under the default threshold, so no direction is claimed.
        self.assertEqual(
            self.summarise([350.0, 355.0, 0.0, 5.0, 10.0])["direction"], "unclear"
        )

    def test_reports_coverage_of_a_full_turn(self):
        self.assertTrue(self.summarise([i * 10.0 for i in range(36)])["full_turn"])
        self.assertFalse(self.summarise([i * 1.0 for i in range(36)])["full_turn"])

    def test_a_wobble_in_place_has_no_clear_direction(self):
        report = self.summarise([10.0, 12.0, 9.0, 11.0, 10.0])
        self.assertEqual(report["direction"], "unclear")

    def test_needs_at_least_two_headings(self):
        with self.assertRaises(OrientationError):
            self.summarise([12.0])


if __name__ == "__main__":
    unittest.main()
