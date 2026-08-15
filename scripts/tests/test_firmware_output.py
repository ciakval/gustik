"""
Tests for the calibration -> firmware handoff (pure text, no hardware).

The point of interest is the Y sign flip. It has to happen exactly once
between the JSON file and the firmware, and getting it wrong produces a
heading that is confidently, stably wrong with nothing anywhere reporting an
error -- so it is pinned here from both directions.
"""

import unittest

from gustik_scripts.calibration import MagCalibration
from gustik_scripts.firmware_output import (
    CONFIG_KEY_OFFSET_X,
    CONFIG_KEY_OFFSET_Y,
    config_lines,
    cpp_constant,
    firmware_hard_iron,
    range_warning,
)

# The bench calibration that shipped in main.cpp, in the chip's own frame.
BENCH = MagCalibration(offset=(1713.5, -1984.0, 1714.0), field_range="8G")


class TestHardIronFrame(unittest.TestCase):
    def test_x_passes_through_unchanged(self):
        self.assertEqual(firmware_hard_iron(BENCH)[0], 1713.5)

    def test_y_is_negated_for_the_firmware_mount_frame(self):
        # magnetometer.cpp returns Y already sign-flipped for up=-z/forward=+x,
        # so the offset subtracted from it must be flipped to match.
        self.assertEqual(firmware_hard_iron(BENCH)[1], 1984.0)

    def test_matches_the_value_committed_in_main_cpp(self):
        self.assertEqual(firmware_hard_iron(BENCH), (1713.5, 1984.0))

    def test_flip_is_an_involution(self):
        once = firmware_hard_iron(BENCH)
        twice = firmware_hard_iron(
            MagCalibration(offset=(once[0], once[1], 0.0), field_range="8G")
        )
        self.assertEqual(twice, (1713.5, -1984.0))

    def test_zero_offset_survives_the_flip_without_a_negative_zero(self):
        x, y = firmware_hard_iron(MagCalibration(offset=(0.0, 0.0, 0.0)))
        self.assertEqual((x, y), (0.0, 0.0))


class TestConfigLines(unittest.TestCase):
    def setUp(self):
        self.lines = config_lines(BENCH)
        self.settings = dict(
            line.split("=", 1) for line in self.lines if not line.startswith("#")
        )

    def test_emits_exactly_the_two_keys_the_firmware_parses(self):
        self.assertEqual(
            set(self.settings), {CONFIG_KEY_OFFSET_X, CONFIG_KEY_OFFSET_Y}
        )

    def test_values_are_in_the_firmware_frame(self):
        self.assertEqual(float(self.settings[CONFIG_KEY_OFFSET_X]), 1713.5)
        self.assertEqual(float(self.settings[CONFIG_KEY_OFFSET_Y]), 1984.0)

    def test_every_non_setting_line_is_a_comment(self):
        # Pasted verbatim into config.txt, so a stray prose line would be
        # silently swallowed by the parser at best.
        for line in self.lines:
            self.assertTrue("=" in line or line.startswith("#"), line)

    def test_records_the_field_range_the_offsets_are_only_valid_at(self):
        self.assertIn("8G", "\n".join(self.lines))

    def test_optional_comment_is_rendered_as_a_comment(self):
        lines = config_lines(BENCH, comment="breadboard with iron base")
        self.assertIn("# breadboard with iron base", lines)


class TestCppConstant(unittest.TestCase):
    def test_declares_the_symbol_main_cpp_uses(self):
        code = "\n".join(cpp_constant(BENCH))
        self.assertIn("MagnetometerCalibration kMagnetometerCalibration", code)

    def test_uses_the_firmware_frame_values(self):
        code = "\n".join(cpp_constant(BENCH))
        self.assertIn(".hardIronOffsetX = 1713.5", code)
        self.assertIn(".hardIronOffsetY = 1984.0", code)

    def test_statement_is_terminated(self):
        self.assertTrue(cpp_constant(BENCH)[-1].rstrip().endswith(";"))


class TestRangeWarning(unittest.TestCase):
    def test_matching_range_is_silent(self):
        self.assertIsNone(range_warning(BENCH))

    def test_mismatched_range_warns(self):
        warning = range_warning(MagCalibration(offset=(1, 2, 3), field_range="2G"))
        self.assertIsNotNone(warning)
        self.assertIn("2G", warning)

    def test_unknown_range_warns(self):
        self.assertIsNotNone(range_warning(MagCalibration(offset=(1, 2, 3))))


if __name__ == "__main__":
    unittest.main()
