#include <unity.h>
#include <cmath>
#include "correct/wind_direction.h"

void setUp(void) {}
void tearDown(void) {}

void test_corrects_raw_vane_octant_by_yaw(void) {
    // boat pointing due north (yaw=0): raw vane reading is already
    // north-relative, so corrected octant equals raw octant unchanged.
    TEST_ASSERT_EQUAL_INT(3, correctWindDirectionOctant(3, 0.0));
}

void test_corrects_raw_vane_octant_with_45deg_yaw(void) {
    // boat yawed 45 degrees (1 octant) clockwise from north: a wind the
    // vane reads as octant 0 (dead ahead) is actually octant 1 relative to
    // true north.
    TEST_ASSERT_EQUAL_INT(1, correctWindDirectionOctant(0, 45.0));
}

void test_correction_wraps_around_octant_boundary(void) {
    // raw octant 7 + yaw octant 2 (90deg) wraps to octant 1, not 9.
    TEST_ASSERT_EQUAL_INT(1, correctWindDirectionOctant(7, 90.0));
}

void test_corrected_octant_stable_while_boat_yaws_under_fixed_true_wind(void) {
    // True wind is fixed at octant 3 relative to north. As the boat swings
    // on its mooring (yaw changes), the vane (mounted on the boat) reads a
    // correspondingly different raw octant - the correction must cancel
    // that out and always report the same true-wind octant (FR-2's whole
    // point: corrected direction must not drift just because the boat spun).
    const int trueWindOctant = 3;
    const double yaws[] = {0.0, 45.0, 90.0, 135.0, 180.0, 225.0, 270.0, 315.0};

    for (double yaw : yaws) {
        int yawOctant = ((static_cast<int>(std::lround(yaw / 45.0)) % 8) + 8) % 8;
        int simulatedRawVane = ((trueWindOctant - yawOctant) % 8 + 8) % 8;
        int corrected = correctWindDirectionOctant(simulatedRawVane, yaw);
        TEST_ASSERT_EQUAL_INT(trueWindOctant, corrected);
    }
}

void test_heading_uses_configurable_calibration_not_hardcoded(void) {
    // same raw XY, two different calibrations -> different headings. Proves
    // the function takes calibration as data, not baked-in magic numbers
    // (AC3 - calibration constants must be configurable).
    MagnetometerCalibration uncalibrated{.hardIronOffsetX = 0.0, .hardIronOffsetY = 0.0};
    MagnetometerCalibration offset{.hardIronOffsetX = 10.0, .hardIronOffsetY = 0.0};

    double headingA = magnetometerHeadingDegrees(5.0, 5.0, uncalibrated);
    double headingB = magnetometerHeadingDegrees(5.0, 5.0, offset);

    TEST_ASSERT_NOT_EQUAL(headingA, headingB);
}

void test_heading_is_within_0_360_range(void) {
    MagnetometerCalibration cal{.hardIronOffsetX = 0.0, .hardIronOffsetY = 0.0};
    double heading = magnetometerHeadingDegrees(-3.0, -7.0, cal);
    TEST_ASSERT_TRUE(heading >= 0.0 && heading < 360.0);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_corrects_raw_vane_octant_by_yaw);
    RUN_TEST(test_corrects_raw_vane_octant_with_45deg_yaw);
    RUN_TEST(test_correction_wraps_around_octant_boundary);
    RUN_TEST(test_corrected_octant_stable_while_boat_yaws_under_fixed_true_wind);
    RUN_TEST(test_heading_uses_configurable_calibration_not_hardcoded);
    RUN_TEST(test_heading_is_within_0_360_range);
    return UNITY_END();
}
