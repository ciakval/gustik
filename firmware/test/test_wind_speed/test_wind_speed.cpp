#include <unity.h>
#include <cmath>
#include "correct/wind_speed.h"

void setUp(void) {}
void tearDown(void) {}

void test_converts_pulses_to_wind_speed_using_calibration(void) {
    AnemometerCalibration cal{.metersPerSecondPerHz = 1.2};
    // 10 pulses over 5s = 2 Hz -> 2 * 1.2 = 2.4 m/s
    double speed = pulsesToWindSpeedMs(10, 5.0, cal);
    TEST_ASSERT_EQUAL_DOUBLE(2.4, speed);
}

void test_no_pulses_gives_zero_speed_not_error(void) {
    AnemometerCalibration cal{.metersPerSecondPerHz = 1.2};
    double speed = pulsesToWindSpeedMs(0, 5.0, cal);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, speed);
    TEST_ASSERT_FALSE(std::isnan(speed));
}

void test_zero_interval_never_produces_nan_or_inf(void) {
    AnemometerCalibration cal{.metersPerSecondPerHz = 1.2};
    double speed = pulsesToWindSpeedMs(5, 0.0, cal);
    TEST_ASSERT_FALSE(std::isnan(speed));
    TEST_ASSERT_FALSE(std::isinf(speed));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_converts_pulses_to_wind_speed_using_calibration);
    RUN_TEST(test_no_pulses_gives_zero_speed_not_error);
    RUN_TEST(test_zero_interval_never_produces_nan_or_inf);
    return UNITY_END();
}
