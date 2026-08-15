"""
Tests for parsing the mag_diag serial stream (pure text, no hardware).

The stream is genuinely dirty: the boot-loader talks at a different baud
before the sketch starts, attaching mid-stream splits a line, and the sketch
itself prints '#' diagnostics when I2C fails. Every one of those has to be
skipped without losing the samples around it.
"""

import io
import unittest

from gustik_scripts.esp32_serial import iter_samples, parse_sample


class TestParseSample(unittest.TestCase):
    def test_parses_a_well_formed_line(self):
        self.assertEqual(parse_sample("MAG 123 -456 789"), (123, -456, 789))

    def test_tolerates_surrounding_whitespace(self):
        self.assertEqual(parse_sample("  MAG 1 2 3  "), (1, 2, 3))

    def test_rejects_the_banner(self):
        self.assertIsNone(
            parse_sample("# gustik mag_diag chip=QMC5883P addr=0x2C range=8G")
        )

    def test_rejects_i2c_error_lines(self):
        self.assertIsNone(parse_sample("# I2C short read"))

    def test_rejects_a_truncated_line_from_attaching_mid_stream(self):
        self.assertIsNone(parse_sample("G 123 -456 789"))
        self.assertIsNone(parse_sample("MAG 123 -45"))

    def test_rejects_bootloader_noise(self):
        for noise in ("rst:0x1 (POWERON_RESET),boot:0x13", "\x00\xff garbage", ""):
            self.assertIsNone(parse_sample(noise))

    def test_rejects_non_integer_values(self):
        self.assertIsNone(parse_sample("MAG 1.5 2 3"))
        self.assertIsNone(parse_sample("MAG x y z"))

    def test_rejects_extra_columns(self):
        # A future sketch printing a 4th field must not be read as if the
        # first three still meant the same thing.
        self.assertIsNone(parse_sample("MAG 1 2 3 4"))

    def test_accepts_zero_and_full_scale_values(self):
        self.assertEqual(parse_sample("MAG 0 0 0"), (0, 0, 0))
        self.assertEqual(parse_sample("MAG -32768 32767 0"), (-32768, 32767, 0))


class TestIterSamples(unittest.TestCase):
    def test_keeps_good_samples_around_the_garbage(self):
        stream = io.StringIO(
            "rst:0x1 (POWERON_RESET),boot:0x13\n"
            "AG 9 9 9\n"
            "# gustik mag_diag range=8G\n"
            "MAG 1 2 3\n"
            "# I2C short read\n"
            "MAG 4 5 6\n"
        )
        self.assertEqual(list(iter_samples(stream)), [(1, 2, 3), (4, 5, 6)])

    def test_empty_stream_yields_nothing(self):
        self.assertEqual(list(iter_samples([])), [])

    def test_a_capture_of_only_noise_yields_nothing_rather_than_raising(self):
        self.assertEqual(list(iter_samples(["boot", "# I2C short read", ""])), [])


class TestCalibratesFromACapture(unittest.TestCase):
    """The parser's output has to be directly usable by the calibration math."""

    def test_a_parsed_capture_feeds_from_samples(self):
        import math

        from gustik_scripts.calibration import MagCalibration

        lines = ["# gustik mag_diag range=8G"]
        for step in range(36):
            angle = math.radians(step * 10)
            lines.append(
                f"MAG {round(500 + 200 * math.cos(angle))} "
                f"{round(-300 + 200 * math.sin(angle))} 42"
            )
        lines.append("# I2C short read")

        cal = MagCalibration.from_samples(list(iter_samples(lines)), field_range="8G")
        self.assertAlmostEqual(cal.offset[0], 500, delta=2)
        self.assertAlmostEqual(cal.offset[1], -300, delta=2)


if __name__ == "__main__":
    unittest.main()
