"""Tests for hard-iron / soft-iron calibration (pure math, no hardware)."""

import json
import math
import os
import tempfile
import unittest

from gustik_scripts.calibration import (
    CalibrationError,
    MagCalibration,
    describe_spin,
)


def _circle(offset=(0.0, 0.0, 0.0), radii=(1000.0, 1000.0), z=0.0, n=360):
    """Samples from a full flat rotation, optionally biased and elliptical."""
    out = []
    for i in range(n):
        a = math.radians(i * 360.0 / n)
        out.append(
            (
                offset[0] + radii[0] * math.cos(a),
                offset[1] + radii[1] * math.sin(a),
                offset[2] + z,
            )
        )
    return out


class TestIdentityCalibration(unittest.TestCase):
    def test_default_is_a_no_op(self):
        cal = MagCalibration()
        self.assertEqual(cal.apply((123.0, -456.0, 789.0)), (123.0, -456.0, 789.0))


class TestHardIron(unittest.TestCase):
    def test_recovers_a_known_offset(self):
        cal = MagCalibration.from_samples(_circle(offset=(1840.0, -1883.0, 2630.0)))
        self.assertAlmostEqual(cal.offset[0], 1840.0, places=3)
        self.assertAlmostEqual(cal.offset[1], -1883.0, places=3)

    def test_centres_the_circle_so_heading_sweeps_a_full_turn(self):
        # This is the actual bug: a large offset squeezes atan2 into a narrow arc.
        samples = _circle(offset=(1840.0, -1883.0, 2630.0), radii=(1217.0, 1131.0))

        raw = [math.degrees(math.atan2(y, x)) % 360.0 for x, y, _ in samples]
        self.assertLess(max(raw) - min(raw), 90.0)  # squeezed, as observed on hardware

        cal = MagCalibration.from_samples(samples)
        fixed = sorted(
            math.degrees(math.atan2(v[1], v[0])) % 360.0 for v in map(cal.apply, samples)
        )
        # Every 30-degree sector must be visited.
        buckets = {int(h // 30) for h in fixed}
        self.assertEqual(buckets, set(range(12)))


class TestSoftIron(unittest.TestCase):
    def test_equalises_the_swept_axes(self):
        cal = MagCalibration.from_samples(_circle(radii=(1200.0, 800.0)))
        corrected = [cal.apply(s) for s in _circle(radii=(1200.0, 800.0))]
        norms = [math.hypot(x, y) for x, y, _ in corrected]
        self.assertAlmostEqual(min(norms), max(norms), delta=1.0)

    def test_leaves_an_unswept_axis_unscaled(self):
        # A flat spin never moves the vertical axis, so its span says nothing
        # about sensitivity -- scaling from it would be garbage.
        cal = MagCalibration.from_samples(_circle(radii=(1200.0, 1200.0), z=2630.0))
        self.assertAlmostEqual(cal.scale[2], 1.0, places=6)

    def test_rejects_a_degenerate_sample_set(self):
        with self.assertRaises(CalibrationError):
            MagCalibration.from_samples([(5.0, 5.0, 5.0)] * 10)

    def test_rejects_too_few_samples(self):
        with self.assertRaises(CalibrationError):
            MagCalibration.from_samples(_circle(n=3))


def _sphere(offset=(0.0, 0.0, 0.0), radius=1000.0, n=200):
    """Samples from tumbling the sensor through every orientation."""
    out = []
    golden = math.pi * (3.0 - math.sqrt(5.0))
    for i in range(n):
        z = 1.0 - 2.0 * i / (n - 1)
        r = math.sqrt(max(0.0, 1.0 - z * z))
        a = golden * i
        out.append(
            (
                offset[0] + radius * r * math.cos(a),
                offset[1] + radius * r * math.sin(a),
                offset[2] + radius * z,
            )
        )
    return out


class TestSweptAxes(unittest.TestCase):
    """Which axes the capture actually exercised, and so which offsets to trust."""

    def test_a_level_spin_sweeps_only_two_axes(self):
        cal = MagCalibration.from_samples(_circle(offset=(0.0, 0.0, 2630.0)))
        self.assertEqual(cal.swept, (0, 1))
        self.assertFalse(cal.covers_all_axes)

    def test_a_tumble_sweeps_all_three(self):
        cal = MagCalibration.from_samples(_sphere(offset=(1800.0, -1900.0, 2600.0)))
        self.assertEqual(cal.swept, (0, 1, 2))
        self.assertTrue(cal.covers_all_axes)

    def test_a_tumble_recovers_the_vertical_offset_a_spin_cannot(self):
        # The whole point: only a tumble separates hard iron on the vertical
        # axis from Earth's vertical field component.
        cal = MagCalibration.from_samples(_sphere(offset=(1800.0, -1900.0, 2600.0)))
        self.assertAlmostEqual(cal.offset[2], 2600.0, delta=20.0)

    def test_a_level_spin_cannot_recover_the_vertical_offset(self):
        # Guards the reason detect_up() must refuse a spin-only calibration:
        # the offset it finds is the field component, not the hard iron.
        cal = MagCalibration.from_samples(_circle(offset=(0.0, 0.0, 1600.0)))
        self.assertAlmostEqual(cal.offset[2], 1600.0, delta=1.0)  # pure field, no bias
        self.assertNotIn(2, cal.swept)

    def test_default_calibration_claims_all_axes(self):
        self.assertTrue(MagCalibration().covers_all_axes)


class TestRoundTrip(unittest.TestCase):
    def test_save_then_load_preserves_swept_axes(self):
        cal = MagCalibration.from_samples(_circle(offset=(0.0, 0.0, 2630.0)))
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "cal.json")
            cal.save(path)
            self.assertEqual(MagCalibration.load(path).swept, (0, 1))

    def test_save_then_load_preserves_values(self):
        cal = MagCalibration(offset=(1.5, -2.5, 3.5), scale=(1.0, 1.1, 1.2))
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "cal.json")
            cal.save(path)
            self.assertEqual(MagCalibration.load(path), cal)

    def test_saved_file_is_readable_json_with_named_fields(self):
        cal = MagCalibration(offset=(1.0, 2.0, 3.0), scale=(1.0, 1.0, 1.0))
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "cal.json")
            cal.save(path)
            with open(path) as f:
                data = json.load(f)
        self.assertEqual(data["offset"], [1.0, 2.0, 3.0])
        self.assertEqual(data["scale"], [1.0, 1.0, 1.0])

    def test_load_rejects_a_malformed_file(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "cal.json")
            with open(path, "w") as f:
                json.dump({"offset": [1.0, 2.0]}, f)
            with self.assertRaises(CalibrationError):
                MagCalibration.load(path)


class TestDescribeSpin(unittest.TestCase):
    def test_reports_spans_and_full_coverage(self):
        report = describe_spin(_circle(offset=(1840.0, -1883.0, 0.0)))
        self.assertEqual(report["count"], 360)
        self.assertAlmostEqual(report["span"][0], 2000.0, delta=20.0)
        self.assertEqual(report["sectors_covered"], 12)
        self.assertTrue(report["full_turn"])

    def test_names_the_unswept_axis_as_the_vertical_candidate(self):
        # A level spin identifies the vertical axis without assuming anything
        # about the local dip angle: it is simply the axis that never moves.
        report = describe_spin(_circle(offset=(1840.0, -1883.0, 2630.0)))
        self.assertEqual(report["vertical_candidate"], "z")

    def test_has_no_vertical_candidate_after_a_tumble(self):
        report = describe_spin(_sphere(offset=(1800.0, -1900.0, 2600.0)))
        self.assertIsNone(report["vertical_candidate"])

    def test_flags_an_incomplete_turn(self):
        quarter = _circle(n=360)[:90]
        report = describe_spin(quarter)
        self.assertLess(report["sectors_covered"], 12)
        self.assertFalse(report["full_turn"])


if __name__ == "__main__":
    unittest.main()
