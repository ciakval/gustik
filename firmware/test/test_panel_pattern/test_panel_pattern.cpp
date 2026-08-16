#include <unity.h>
#include <cstdio>
#include "indicate/pattern.h"

void setUp(void) {}
void tearDown(void) {}

void test_off_is_never_lit(void) {
    LanePattern off = lanePatternOff();
    TEST_ASSERT_FALSE(isLit(off, 0, 0));
    TEST_ASSERT_FALSE(isLit(off, 12345, 0));
    TEST_ASSERT_FALSE(isLit(off, 4294967295UL, 0));
}

void test_solid_is_always_lit(void) {
    LanePattern solid = lanePatternSolid();
    for (unsigned long t = 0; t < 10000; t += 37) {
        TEST_ASSERT_TRUE(isLit(solid, t, 0));
    }
}

void test_slow_blink_is_1hz_half_duty(void) {
    LanePattern p = lanePatternSlowBlink();
    TEST_ASSERT_TRUE(isLit(p, 0, 0));
    TEST_ASSERT_TRUE(isLit(p, 499, 0));
    TEST_ASSERT_FALSE(isLit(p, 500, 0));
    TEST_ASSERT_FALSE(isLit(p, 999, 0));
    TEST_ASSERT_TRUE(isLit(p, 1000, 0));  // next cycle
}

void test_fast_blink_is_4hz(void) {
    LanePattern p = lanePatternFastBlink();
    TEST_ASSERT_TRUE(isLit(p, 0, 0));
    TEST_ASSERT_FALSE(isLit(p, 125, 0));
    TEST_ASSERT_TRUE(isLit(p, 250, 0));
}

void test_phase0_anchors_the_cycle(void) {
    // A fault code must start at flash 1 the moment the fault appears, not
    // wherever a free-running clock happens to be.
    LanePattern p = lanePatternSlowBlink();
    TEST_ASSERT_TRUE(isLit(p, 8000, 8000));
    TEST_ASSERT_FALSE(isLit(p, 8600, 8000));
}

void test_pulse_is_a_short_flash_per_period(void) {
    LanePattern p = lanePatternPulse(kHeartbeatOnMs, kHeartbeatAwakePeriodMs);
    TEST_ASSERT_TRUE(isLit(p, 0, 0));
    TEST_ASSERT_TRUE(isLit(p, 59, 0));
    TEST_ASSERT_FALSE(isLit(p, 60, 0));
    TEST_ASSERT_FALSE(isLit(p, 2999, 0));
    TEST_ASSERT_TRUE(isLit(p, 3000, 0));
}

void test_double_pulse_flashes_twice_per_period(void) {
    // "alive, and there is a backlog buffered to flash" - one lane, two facts.
    LanePattern p = lanePatternDoublePulse(kHeartbeatOnMs, 120, kHeartbeatAwakePeriodMs);
    TEST_ASSERT_TRUE(isLit(p, 0, 0));
    TEST_ASSERT_FALSE(isLit(p, 60, 0));
    TEST_ASSERT_FALSE(isLit(p, 179, 0));
    TEST_ASSERT_TRUE(isLit(p, 180, 0));  // second flash
    TEST_ASSERT_FALSE(isLit(p, 240, 0));
    TEST_ASSERT_TRUE(isLit(p, 3000, 0));  // next period
}

// Counts the rising edges in one full period - i.e. what someone on the boat
// would actually count.
static int countFlashesInPeriod(const LanePattern &p, unsigned long periodMs) {
    int flashes = 0;
    bool previous = false;
    for (unsigned long t = 0; t < periodMs; t++) {
        bool lit = isLit(p, t, 0);
        if (lit && !previous) {
            flashes++;
        }
        previous = lit;
    }
    return flashes;
}

void test_fault_code_produces_exactly_n_flashes_then_a_pause(void) {
    for (int n = 1; n <= 8; n++) {
        LanePattern p = lanePatternCode(n, kCodeAwakePeriodMs);
        unsigned long period = static_cast<unsigned long>(p.flashes) * (p.onMs + p.offMs) + p.pauseMs;
        char message[64];
        snprintf(message, sizeof(message), "code %d flash count", n);
        TEST_ASSERT_EQUAL_INT_MESSAGE(n, countFlashesInPeriod(p, period), message);
        // The pause must be long enough to read as a gap, or the code is
        // uncountable however correct the flash count is.
        TEST_ASSERT_TRUE_MESSAGE(p.pauseMs >= 2 * kCodeFlashMs, message);
    }
}

void test_fault_code_asleep_repeats_more_slowly_but_counts_the_same(void) {
    LanePattern awake = lanePatternCode(3, kCodeAwakePeriodMs);
    LanePattern asleep = lanePatternCode(3, kCodeAsleepPeriodMs);
    TEST_ASSERT_EQUAL_INT(awake.flashes, asleep.flashes);
    TEST_ASSERT_TRUE(asleep.pauseMs > awake.pauseMs);
    TEST_ASSERT_EQUAL_INT(3, countFlashesInPeriod(asleep, kCodeAsleepPeriodMs));
}

void test_fault_code_of_zero_is_off(void) {
    TEST_ASSERT_TRUE(lanePatternCode(0, kCodeAwakePeriodMs) == lanePatternOff());
    TEST_ASSERT_TRUE(lanePatternCode(-1, kCodeAwakePeriodMs) == lanePatternOff());
}

void test_one_shot_runs_once_then_stays_dark(void) {
    LanePattern p = lanePatternOneShot(2, kBannerFlashMs, kBannerFlashMs);
    TEST_ASSERT_TRUE(isLit(p, 0, 0));
    TEST_ASSERT_FALSE(isLit(p, 100, 0));
    TEST_ASSERT_TRUE(isLit(p, 200, 0));
    TEST_ASSERT_FALSE(isLit(p, 300, 0));
    // ...and never again.
    TEST_ASSERT_FALSE(isLit(p, 400, 0));
    TEST_ASSERT_FALSE(isLit(p, 100000, 0));
}

void test_inverted_pulse_is_a_dark_notch_in_a_lit_lane(void) {
    // Section 5.4: "pulsing means you're on somebody's phone".
    LanePattern p = lanePatternInverted(lanePatternPulse(100, 1000));
    TEST_ASSERT_FALSE(isLit(p, 0, 0));
    TEST_ASSERT_FALSE(isLit(p, 99, 0));
    TEST_ASSERT_TRUE(isLit(p, 100, 0));
    TEST_ASSERT_TRUE(isLit(p, 999, 0));
    TEST_ASSERT_FALSE(isLit(p, 1000, 0));
}

void test_millis_rollover_does_not_stick_a_lane(void) {
    // millis() wraps every ~49 days; unsigned subtraction has to carry the
    // elapsed time across the wrap or a lane freezes for good.
    const unsigned long nearMax = 4294967295UL - 200;
    LanePattern p = lanePatternSlowBlink();
    TEST_ASSERT_TRUE(isLit(p, nearMax + 100, nearMax));   // 100 ms elapsed
    TEST_ASSERT_TRUE(isLit(p, nearMax + 400, nearMax));   // 400 ms, wrapped
    TEST_ASSERT_FALSE(isLit(p, nearMax + 700, nearMax));  // 700 ms, wrapped
}

void test_degenerate_patterns_are_dark_not_undefined(void) {
    // A hand-built pattern with no on and no off time must not divide by
    // zero; every field is public, so this is reachable.
    LanePattern p;
    p.flashes = 3;
    TEST_ASSERT_FALSE(isLit(p, 0, 0));
    TEST_ASSERT_FALSE(isLit(p, 12345, 0));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_off_is_never_lit);
    RUN_TEST(test_solid_is_always_lit);
    RUN_TEST(test_slow_blink_is_1hz_half_duty);
    RUN_TEST(test_fast_blink_is_4hz);
    RUN_TEST(test_phase0_anchors_the_cycle);
    RUN_TEST(test_pulse_is_a_short_flash_per_period);
    RUN_TEST(test_double_pulse_flashes_twice_per_period);
    RUN_TEST(test_fault_code_produces_exactly_n_flashes_then_a_pause);
    RUN_TEST(test_fault_code_asleep_repeats_more_slowly_but_counts_the_same);
    RUN_TEST(test_fault_code_of_zero_is_off);
    RUN_TEST(test_one_shot_runs_once_then_stays_dark);
    RUN_TEST(test_inverted_pulse_is_a_dark_notch_in_a_lit_lane);
    RUN_TEST(test_millis_rollover_does_not_stick_a_lane);
    RUN_TEST(test_degenerate_patterns_are_dark_not_undefined);
    return UNITY_END();
}
