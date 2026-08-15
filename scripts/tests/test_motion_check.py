"""
Tests for the "did anyone actually turn the board?" check.

This guards a false pass that describe_spin() cannot catch on its own. It
normalises each axis by its own span before counting sectors, so a stationary
sensor's noise scatters right around the circle and reports a confident
"12/12 sectors, all three axes swept" -- and the offsets derived from it are
just wherever the sensor was pointing. The resulting heading is stable and
confidently wrong, with nothing anywhere reporting an error.

The stationary numbers below are a real 241-sample capture from the ESP32 with
the board sitting still on the bench.
"""

import math
import unittest

from gustik_scripts.calibration import describe_spin
from gustik_scripts.firmware_output import LSB_PER_GAUSS, motion_warning


def _rotation(radius, samples=120, centre=(0.0, 0.0), vertical=1456.0):
    """A level turn of the given peak amplitude, in the x-y plane."""
    out = []
    for step in range(samples):
        angle = 2 * math.pi * step / samples
        out.append(
            (
                centre[0] + radius * math.cos(angle),
                centre[1] + radius * math.sin(angle),
                vertical,
            )
        )
    return out


def _stationary(samples=241, noise=20.0):
    """A sensor sitting still: small, roughly symmetric jitter on every axis."""
    out = []
    for step in range(samples):
        wobble = math.sin(step * 2.4)
        out.append((429.5 + noise * wobble, 231.5 + noise * math.cos(step * 1.7),
                    1456.0 + noise * math.sin(step * 0.9)))
    return out


class TestStationaryCaptureIsRejected(unittest.TestCase):
    def setUp(self):
        self.report = describe_spin(_stationary())

    def test_describe_spin_alone_would_have_passed_it(self):
        # The premise of this whole check: coverage looks perfect.
        self.assertEqual(self.report["sectors_covered"], self.report["sectors"])
        self.assertTrue(self.report["full_turn"])

    def test_motion_warning_catches_it(self):
        warning = motion_warning(self.report, "8G")
        self.assertIsNotNone(warning)
        self.assertIn("stationary", warning)

    def test_the_real_bench_capture_span_is_far_below_threshold(self):
        # Measured spans were x=21, y=29, z=40 LSB against a 300 LSB threshold.
        self.assertLess(max(self.report["span"]), 0.08 * LSB_PER_GAUSS["8G"])


class TestRealRotationIsAccepted(unittest.TestCase):
    def test_a_nominal_czech_level_turn_passes(self):
        # ~0.20 G horizontal component -> 750 LSB amplitude at 8G.
        report = describe_spin(_rotation(750.0))
        self.assertIsNone(motion_warning(report, "8G"))

    def test_passes_even_with_a_large_hard_iron_offset(self):
        # The offset moves the circle; it does not shrink it.
        report = describe_spin(_rotation(750.0, centre=(1713.5, -1984.0)))
        self.assertIsNone(motion_warning(report, "8G"))

    def test_passes_a_heavily_soft_iron_suppressed_turn(self):
        # Half the nominal field still clears the threshold, so an iron base
        # that weakens the signal does not trigger a false alarm.
        report = describe_spin(_rotation(375.0))
        self.assertIsNone(motion_warning(report, "8G"))

    def test_threshold_sits_between_the_two_regimes(self):
        # A quarter-strength turn (187 LSB amplitude, 375 span) still passes;
        # bench noise (40 span) does not. Confirms the margin is real.
        self.assertIsNone(motion_warning(describe_spin(_rotation(190.0)), "8G"))
        self.assertIsNotNone(motion_warning(describe_spin(_stationary()), "8G"))


class TestRangeScaling(unittest.TestCase):
    def test_threshold_scales_with_field_range(self):
        # 400 LSB is a real turn at 30G (0.40 G) but noise at 2G (0.027 G).
        report = describe_spin(_rotation(200.0))
        self.assertIsNone(motion_warning(report, "30G"))
        self.assertIsNotNone(motion_warning(report, "2G"))

    def test_unknown_range_does_not_raise(self):
        self.assertIsNone(motion_warning(describe_spin(_rotation(750.0)), None))
        self.assertIsNone(motion_warning(describe_spin(_stationary()), "not-a-range"))


if __name__ == "__main__":
    unittest.main()
