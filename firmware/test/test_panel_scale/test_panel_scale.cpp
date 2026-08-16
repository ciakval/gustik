#include <unity.h>
#include <cmath>
#include "indicate/panel.h"

void setUp(void) {}
void tearDown(void) {}

void test_wind_bands_match_beaufort_js(void) {
    // Same boundaries as backend/src/static/beaufort.js, so the LEDs, the
    // dashboard and the Beaufort label never disagree about the same wind.
    TEST_ASSERT_EQUAL_INT(0, windDetailPosition(0.0));
    TEST_ASSERT_EQUAL_INT(0, windDetailPosition(1.59));
    TEST_ASSERT_EQUAL_INT(1, windDetailPosition(1.6));
    TEST_ASSERT_EQUAL_INT(1, windDetailPosition(3.39));
    TEST_ASSERT_EQUAL_INT(2, windDetailPosition(3.4));
    TEST_ASSERT_EQUAL_INT(2, windDetailPosition(5.49));
    TEST_ASSERT_EQUAL_INT(3, windDetailPosition(5.5));
    TEST_ASSERT_EQUAL_INT(3, windDetailPosition(7.99));
    TEST_ASSERT_EQUAL_INT(4, windDetailPosition(8.0));
    TEST_ASSERT_EQUAL_INT(4, windDetailPosition(25.0));
}

void test_wind_position_is_total(void) {
    // A negative or absurd speed must still land on the scale rather than
    // indexing off the end of the detail group.
    const double inputs[] = {-1.0, -1e9, 0.0, 1e9, NAN};
    for (double value : inputs) {
        int position = windDetailPosition(value);
        TEST_ASSERT_TRUE(position >= 0 && position < kDetailPositionCount);
    }
}

void test_beaufort_6_is_the_capsize_cue(void) {
    // The panel's one genuine safety signal: position 5 stops merely lighting
    // and starts fast-blinking at the wind that capsizes a P550.
    TEST_ASSERT_FALSE(windIsStrong(10.79));
    TEST_ASSERT_TRUE(windIsStrong(10.8));
    TEST_ASSERT_TRUE(windIsStrong(20.0));
    // ...and it is always at the red end of the ramp when it fires.
    TEST_ASSERT_EQUAL_INT(kDetailPositionCount - 1, windDetailPosition(10.8));
}

void test_rssi_bands_match_the_status_page_reference_lines(void) {
    TEST_ASSERT_EQUAL_INT(0, signalDetailPosition(true, true, -30));
    TEST_ASSERT_EQUAL_INT(0, signalDetailPosition(true, true, -55));
    TEST_ASSERT_EQUAL_INT(1, signalDetailPosition(true, true, -56));
    TEST_ASSERT_EQUAL_INT(1, signalDetailPosition(true, true, -67));  // the -67 line
    TEST_ASSERT_EQUAL_INT(2, signalDetailPosition(true, true, -68));
    TEST_ASSERT_EQUAL_INT(2, signalDetailPosition(true, true, -80));  // the -80 line
    TEST_ASSERT_EQUAL_INT(3, signalDetailPosition(true, true, -81));
    TEST_ASSERT_EQUAL_INT(3, signalDetailPosition(true, true, -90));
    TEST_ASSERT_EQUAL_INT(4, signalDetailPosition(true, true, -91));
}

void test_no_signal_is_the_worst_position(void) {
    TEST_ASSERT_EQUAL_INT(4, signalDetailPosition(false, true, -30));
    TEST_ASSERT_EQUAL_INT(4, signalDetailPosition(true, false, -30));
    TEST_ASSERT_EQUAL_INT(4, signalDetailPosition(false, false, 0));
}

void test_green_is_at_the_good_end_of_both_ramps(void) {
    // Position 0 and 1 are the green LEDs, 2 and 3 yellow, 4 red. Both
    // scales must put "fine" at the green end - a fill lit from the red end
    // would have signal backwards relative to wind, which is why the detail
    // group is a single dot and not a bar. This asserts the direction.
    TEST_ASSERT_TRUE(windDetailPosition(0.5) < windDetailPosition(9.0));
    TEST_ASSERT_TRUE(signalDetailPosition(true, true, -40) <
                     signalDetailPosition(true, true, -85));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_wind_bands_match_beaufort_js);
    RUN_TEST(test_wind_position_is_total);
    RUN_TEST(test_beaufort_6_is_the_capsize_cue);
    RUN_TEST(test_rssi_bands_match_the_status_page_reference_lines);
    RUN_TEST(test_no_signal_is_the_worst_position);
    RUN_TEST(test_green_is_at_the_good_end_of_both_ramps);
    return UNITY_END();
}
