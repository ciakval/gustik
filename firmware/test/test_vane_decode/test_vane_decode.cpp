#include <unity.h>
#include "sense/vane_decode.h"

void setUp(void) {}
void tearDown(void) {}

// The 8 primary detents must decode exactly - these are the measured anchors
// from the real vane (see sense/vane_decode.cpp for provenance).
void test_primary_detents_decode_exactly(void) {
    TEST_ASSERT_EQUAL_INT(0, vaneOctantForAdc(2943)); //   0deg
    TEST_ASSERT_EQUAL_INT(1, vaneOctantForAdc(1663)); //  45deg
    TEST_ASSERT_EQUAL_INT(2, vaneOctantForAdc(209));  //  90deg
    TEST_ASSERT_EQUAL_INT(3, vaneOctantForAdc(572));  // 135deg
    TEST_ASSERT_EQUAL_INT(4, vaneOctantForAdc(974));  // 180deg
    TEST_ASSERT_EQUAL_INT(5, vaneOctantForAdc(2315)); // 225deg
    TEST_ASSERT_EQUAL_INT(6, vaneOctantForAdc(3855)); // 270deg
    TEST_ASSERT_EQUAL_INT(7, vaneOctantForAdc(3465)); // 315deg
}

// bug-050 regression. With only the 8 primary anchors, these three half-
// detents decoded to 90deg, 0deg and 225deg respectively - errors of 67.5,
// 67.5 and 112.5 degrees at mechanically stable rest positions. Each must now
// land on one of its two true neighbours.
void test_half_detent_157_5_does_not_decode_as_90deg(void) {
    TEST_ASSERT_EQUAL_INT(4, vaneOctantForAdc(339)); // 135deg or 180deg, not 90deg
}

void test_half_detent_292_5_does_not_decode_as_0deg(void) {
    TEST_ASSERT_EQUAL_INT(7, vaneOctantForAdc(3134)); // 270deg or 315deg, not 0deg
}

void test_half_detent_337_5_does_not_decode_as_225deg(void) {
    TEST_ASSERT_EQUAL_INT(0, vaneOctantForAdc(2604)); // 315deg or 0deg, not 225deg
}

// The remaining five half-detents already decoded acceptably before the fix;
// they are pinned so a future table edit cannot silently regress them.
void test_remaining_half_detents_decode_to_a_true_neighbour(void) {
    TEST_ASSERT_EQUAL_INT(1, vaneOctantForAdc(1442)); //  22.5deg -> 0 or 1
    TEST_ASSERT_EQUAL_INT(2, vaneOctantForAdc(175));  //  67.5deg -> 1 or 2
    TEST_ASSERT_EQUAL_INT(3, vaneOctantForAdc(107));  // 112.5deg -> 2 or 3
    TEST_ASSERT_EQUAL_INT(5, vaneOctantForAdc(803));  // 202.5deg -> 4 or 5
    TEST_ASSERT_EQUAL_INT(6, vaneOctantForAdc(2195)); // 247.5deg -> 5 or 6
}

// Real readings jitter a few counts around each anchor (measured per-detent
// spread was 2-6 counts on the primaries, with noise tails up to ~40).
void test_tolerates_adc_jitter_around_each_primary(void) {
    for (int delta = -20; delta <= 20; delta += 5) {
        TEST_ASSERT_EQUAL_INT(0, vaneOctantForAdc(2943 + delta));
        TEST_ASSERT_EQUAL_INT(4, vaneOctantForAdc(974 + delta));
        TEST_ASSERT_EQUAL_INT(6, vaneOctantForAdc(3855 + delta));
    }
}

// Total function: the rails and anything out of range still return a valid
// octant rather than reading off the end of the table. A railed pin means
// broken wiring (that failure is diagnosed by the boot-time Serial output,
// not here), but it must not be undefined behaviour.
void test_out_of_range_readings_return_a_valid_octant(void) {
    const int inputs[] = {-1000, -1, 0, 4095, 9999};
    for (unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        int octant = vaneOctantForAdc(inputs[i]);
        TEST_ASSERT_TRUE(octant >= 0 && octant < 8);
    }
}

// Every octant must be reachable - a table typo that dropped or duplicated an
// octant would otherwise pass every test above.
void test_every_octant_is_reachable(void) {
    bool seen[8] = {false};
    for (int adc = 0; adc <= 4095; adc++) {
        seen[vaneOctantForAdc(adc)] = true;
    }
    for (int octant = 0; octant < 8; octant++) {
        TEST_ASSERT_TRUE(seen[octant]);
    }
}

// The decode is total by design, so an unwired vane still produces a
// confident octant - only the raw reading can say the sensor is not there.
void test_open_and_shorted_wiring_are_reported_as_implausible(void) {
    TEST_ASSERT_FALSE(vaneAdcPlausible(0));      // pin shorted to GND
    TEST_ASSERT_FALSE(vaneAdcPlausible(4095));   // open, pulled to 3V3
    TEST_ASSERT_FALSE(vaneAdcPlausible(-1));
}

void test_every_measured_detent_is_plausible(void) {
    // The measured anchors span 107..3855; every one of the 16 real rest
    // positions must be inside the band, or the panel cries wolf on a
    // working sensor.
    const int detents[] = {107, 175, 209, 339, 572, 803, 974, 1442,
                           1663, 2195, 2315, 2604, 2943, 3134, 3465, 3855};
    for (int adc : detents) {
        TEST_ASSERT_TRUE(vaneAdcPlausible(adc));
    }
}

void test_intermediate_readings_stay_plausible(void) {
    // A turning vane passes through genuinely intermediate resistances. An
    // anchor-proximity check would false-alarm on those; a range check does
    // not, which is why this is a range check.
    for (int adc = kVaneAdcMinPlausible; adc <= kVaneAdcMaxPlausible; adc += 7) {
        TEST_ASSERT_TRUE(vaneAdcPlausible(adc));
    }
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_open_and_shorted_wiring_are_reported_as_implausible);
    RUN_TEST(test_every_measured_detent_is_plausible);
    RUN_TEST(test_intermediate_readings_stay_plausible);
    RUN_TEST(test_primary_detents_decode_exactly);
    RUN_TEST(test_half_detent_157_5_does_not_decode_as_90deg);
    RUN_TEST(test_half_detent_292_5_does_not_decode_as_0deg);
    RUN_TEST(test_half_detent_337_5_does_not_decode_as_225deg);
    RUN_TEST(test_remaining_half_detents_decode_to_a_true_neighbour);
    RUN_TEST(test_tolerates_adc_jitter_around_each_primary);
    RUN_TEST(test_out_of_range_readings_return_a_valid_octant);
    RUN_TEST(test_every_octant_is_reachable);
    return UNITY_END();
}
