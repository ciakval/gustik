#include <unity.h>
#include "indicate/panel.h"

void setUp(void) {}
void tearDown(void) {}

namespace {

// Drives a StatusPanel against a fake clock in 10 ms steps, which is far
// finer than any timing the design depends on and close to the real loop
// rate the panel is ticked at.
struct Harness {
    StatusPanel panel;
    PanelInputs inputs;
    PanelOutputs out;
    unsigned long now = 0;

    void begin(unsigned long sleepTimeoutMs = 0, bool enabled = true) {
        PanelSettings settings;
        settings.enabled = enabled;
        settings.sleepTimeoutMs = sleepTimeoutMs;
        inputs.haveSample = true;
        inputs.configLoaded = true;
        inputs.wifiAssociated = true;
        inputs.rssiValid = true;
        inputs.rssiDbm = -50;
        inputs.lastSendOk = true;
        inputs.lastHttpStatus = 201;
        inputs.hasCounts = true;
        inputs.lastInserted = 1;
        inputs.clockSynced = true;
        inputs.magnetometerOk = true;
        inputs.vaneInRange = true;
        panel.begin(settings, now);
        out = panel.tick(inputs, now, ButtonEvent::None);
    }

    void advance(unsigned long deltaMs) {
        unsigned long target = now + deltaMs;
        while (now < target) {
            now += 10;
            out = panel.tick(inputs, now, ButtonEvent::None);
        }
    }

    void send(ButtonEvent event) {
        now += 10;
        out = panel.tick(inputs, now, event);
    }
};

int statusLanesInUse(const PanelOutputs &out) {
    int count = 0;
    for (int i = 0; i < kStatusLaneCount; i++) {
        if (out.status[i].pattern != lanePatternOff()) {
            count++;
        }
    }
    return count;
}

int detailPositionsInUse(const PanelOutputs &out) {
    int count = 0;
    for (int i = 0; i < kDetailPositionCount; i++) {
        if (out.detail[i].pattern != lanePatternOff()) {
            count++;
        }
    }
    return count;
}

int litCount(const PanelOutputs &out, unsigned long nowMs) {
    int count = 0;
    for (int i = 0; i < kStatusLaneCount; i++) {
        if (isLit(out.status[i], nowMs)) {
            count++;
        }
    }
    for (int i = 0; i < kDetailPositionCount; i++) {
        if (isLit(out.detail[i], nowMs)) {
            count++;
        }
    }
    return count;
}

bool sameStatusGroup(const PanelOutputs &a, const PanelOutputs &b) {
    for (int i = 0; i < kStatusLaneCount; i++) {
        if (a.status[i].pattern != b.status[i].pattern || a.status[i].phase0 != b.status[i].phase0) {
            return false;
        }
    }
    return true;
}

} // namespace

void test_boot_self_test_lights_everything_then_sweeps_the_row(void) {
    Harness h;
    h.begin();
    // All nine on first: a dead LED, a cold joint or a wrong resistor is
    // obvious at power-on.
    h.advance(200);
    TEST_ASSERT_EQUAL_INT(kStatusLaneCount + kDetailPositionCount, litCount(h.out, h.now));

    // Then exactly one at a time, left to right across the whole row - which
    // teaches the row's order without anybody reading anything.
    for (int step = 0; step < kStatusLaneCount + kDetailPositionCount; step++) {
        h.now = kSelfTestAllOnMs + step * kSelfTestStepMs + kSelfTestStepMs / 2;
        h.out = h.panel.tick(h.inputs, h.now, ButtonEvent::None);
        TEST_ASSERT_EQUAL_INT(1, litCount(h.out, h.now));
        if (step < kStatusLaneCount) {
            TEST_ASSERT_TRUE(isLit(h.out.status[step], h.now));
        } else {
            TEST_ASSERT_TRUE(isLit(h.out.detail[step - kStatusLaneCount], h.now));
        }
    }
    TEST_ASSERT_TRUE(h.panel.isSelfTesting(kSelfTestTotalMs - 1));
    TEST_ASSERT_FALSE(h.panel.isSelfTesting(kSelfTestTotalMs));
}

void test_wind_is_the_default_mode(void) {
    // With nobody touching anything the panel answers both "is it working"
    // and "how windy is it".
    Harness h;
    h.begin();
    h.inputs.windSpeedMs = 4.0;
    h.advance(2500);
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Wind);
    TEST_ASSERT_EQUAL_INT(1, detailPositionsInUse(h.out));
    TEST_ASSERT_TRUE(h.out.detail[2].pattern != lanePatternOff());
}

void test_short_press_cycles_the_detail_modes(void) {
    Harness h;
    h.begin();
    h.advance(2000);
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Wind);
    h.send(ButtonEvent::Short);
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Signal);
    h.send(ButtonEvent::Short);
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Sensors);
    h.send(ButtonEvent::Short);
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Wind);
}

void test_mode_banner_is_confined_to_the_detail_group(void) {
    // A banner across the whole row would look like the low-battery alarm,
    // and the status group must never flash for a reason that is not status.
    Harness h;
    h.begin();
    h.advance(2000);
    PanelOutputs before = h.out;
    h.send(ButtonEvent::Short);  // -> signal, banner of 2 flashes
    h.advance(50);
    for (int i = 0; i < kDetailPositionCount; i++) {
        TEST_ASSERT_TRUE(h.out.detail[i].pattern != lanePatternOff());
        TEST_ASSERT_TRUE(h.out.detail[i].pattern.oneShot);
    }
    TEST_ASSERT_TRUE(sameStatusGroup(before, h.out));
}

void test_the_status_group_is_identical_in_every_detail_mode(void) {
    // The invariant the whole two-group layout exists to protect: going to
    // look at the wind, or the signal, can never hide a fault.
    Harness h;
    h.begin();
    h.inputs.windDirOctant = 0;  // shortest direction code, so it clears fast
    h.advance(2000);             // past the self-test and the first direction code
    PanelOutputs inWind = h.out;

    h.send(ButtonEvent::Short);
    h.advance(600);  // past the 2-flash banner
    PanelOutputs inSignal = h.out;

    h.send(ButtonEvent::Short);
    h.advance(800);  // past the 3-flash banner
    PanelOutputs inSensors = h.out;

    h.send(ButtonEvent::Short);
    h.advance(1200);  // back to wind, past its banner and direction code
    PanelOutputs backInWind = h.out;

    TEST_ASSERT_TRUE(sameStatusGroup(inWind, inSignal));
    TEST_ASSERT_TRUE(sameStatusGroup(inWind, inSensors));
    TEST_ASSERT_TRUE(sameStatusGroup(inWind, backInWind));
}

void test_a_fault_stays_visible_while_someone_looks_at_the_wind(void) {
    Harness h;
    h.begin();
    h.inputs.clockSynced = false;  // code 7
    h.advance(2000);
    TEST_ASSERT_EQUAL_INT(7, h.out.status[kStatusRed].pattern.flashes);
    h.send(ButtonEvent::Short);
    h.advance(600);
    TEST_ASSERT_EQUAL_INT(7, h.out.status[kStatusRed].pattern.flashes);
    h.send(ButtonEvent::Short);
    h.advance(800);
    TEST_ASSERT_EQUAL_INT(7, h.out.status[kStatusRed].pattern.flashes);
}

void test_exactly_one_detail_position_is_used_by_wind_and_signal(void) {
    // The dot rule. Section 8.1's renderer deliberately does not enforce it -
    // it is a property of how each mode fills the array - so it is asserted
    // here rather than assumed.
    Harness h;
    h.begin();
    h.advance(2000);
    const double speeds[] = {0.0, 2.0, 4.0, 6.0, 9.0, 12.0, -3.0};
    for (double speed : speeds) {
        h.inputs.windSpeedMs = speed;
        h.advance(100);
        TEST_ASSERT_EQUAL_INT(1, detailPositionsInUse(h.out));
    }

    h.send(ButtonEvent::Short);
    h.advance(600);
    const int rssis[] = {-30, -60, -70, -85, -100};
    for (int rssi : rssis) {
        h.inputs.rssiDbm = rssi;
        h.advance(100);
        TEST_ASSERT_EQUAL_INT(1, detailPositionsInUse(h.out));
    }
}

void test_sensor_mode_uses_positions_positionally(void) {
    // The one deliberate exception to the severity metaphor: three
    // independent sensors are not a scale. Acceptable because it is a
    // diagnostic entered on purpose, never the resting default.
    Harness h;
    h.begin();
    h.advance(2000);
    h.send(ButtonEvent::Short);
    h.send(ButtonEvent::Short);
    h.advance(800);
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Sensors);
    // Vane and magnetometer both healthy: solid, and position 5 dark.
    TEST_ASSERT_TRUE(h.out.detail[1].pattern == lanePatternSolid());
    TEST_ASSERT_TRUE(h.out.detail[2].pattern == lanePatternSolid());
    TEST_ASSERT_TRUE(h.out.detail[3].pattern == lanePatternOff());
    TEST_ASSERT_TRUE(h.out.detail[4].pattern == lanePatternOff());

    h.inputs.magnetometerOk = false;
    h.advance(50);
    TEST_ASSERT_TRUE(h.out.detail[2].pattern == lanePatternSlowBlink());
    TEST_ASSERT_TRUE(h.out.detail[4].pattern == lanePatternSlowBlink());
}

void test_anemometer_pulses_flash_position_one_live(void) {
    // bug-059 in one line: dark while the cups are turning means the
    // anemometer is not wired.
    Harness h;
    h.begin();
    h.advance(2000);
    h.send(ButtonEvent::Short);
    h.send(ButtonEvent::Short);
    h.advance(800);
    TEST_ASSERT_TRUE(h.out.detail[0].pattern == lanePatternOff());  // no pulse yet

    h.inputs.pulseCountSnapshot = 1;
    h.send(ButtonEvent::None);
    TEST_ASSERT_TRUE(h.out.detail[0].pattern != lanePatternOff());
    TEST_ASSERT_TRUE(isLit(h.out.detail[0], h.now));
}

void test_a_counter_reset_is_a_resync_not_a_pulse(void) {
    // The sample cycle zeroes the counter every 3 s. A decrease must not
    // read as a reed closure, or the indicator flashes on its own forever.
    Harness h;
    h.begin();
    h.advance(2000);
    h.send(ButtonEvent::Short);
    h.send(ButtonEvent::Short);
    h.advance(800);

    h.inputs.pulseCountSnapshot = 41;  // a real reed closure
    h.send(ButtonEvent::None);
    TEST_ASSERT_TRUE(isLit(h.out.detail[0], h.now));
    h.advance(200);  // the 30 ms flash is over
    TEST_ASSERT_FALSE(isLit(h.out.detail[0], h.now));

    h.inputs.pulseCountSnapshot = 0;  // readAndResetPulseCount() ran
    h.advance(200);
    TEST_ASSERT_FALSE(isLit(h.out.detail[0], h.now));

    // ...and counting picks straight back up from the new baseline.
    h.inputs.pulseCountSnapshot = 1;
    h.send(ButtonEvent::None);
    TEST_ASSERT_TRUE(isLit(h.out.detail[0], h.now));
}

void test_a_non_default_mode_returns_to_wind_by_itself(void) {
    Harness h;
    h.begin(/*sleepTimeoutMs=*/0);  // never sleep, so only auto-return can act
    h.advance(2000);
    h.send(ButtonEvent::Short);
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Signal);
    h.advance(kModeAutoReturnMs - 1000);
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Signal);
    h.advance(2000);
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Wind);
}

void test_the_panel_sleeps_to_a_heartbeat_and_a_fault_code(void) {
    Harness h;
    h.begin(/*sleepTimeoutMs=*/5000);
    h.inputs.clockSynced = false;  // code 7
    h.advance(6000);
    TEST_ASSERT_TRUE(h.panel.isAsleep());
    // Nothing in the detail group, and the radio and data lanes are dark.
    TEST_ASSERT_EQUAL_INT(0, detailPositionsInUse(h.out));
    TEST_ASSERT_TRUE(h.out.status[kStatusYellow].pattern == lanePatternOff());
    TEST_ASSERT_TRUE(h.out.status[kStatusGreen].pattern == lanePatternOff());
    // But proof of life and the fault survive, both at the slow repeat - so
    // "the whole row dark means not running" stays true while asleep.
    TEST_ASSERT_EQUAL_INT(2, statusLanesInUse(h.out));
    TEST_ASSERT_EQUAL_INT(7, h.out.status[kStatusRed].pattern.flashes);
    TEST_ASSERT_TRUE(h.out.status[kStatusRed].pattern.pauseMs >
                     lanePatternCode(7, kCodeAwakePeriodMs).pauseMs);
    TEST_ASSERT_TRUE(h.out.status[kStatusBlue].pattern.pauseMs >
                     lanePatternPulse(kHeartbeatOnMs, kHeartbeatAwakePeriodMs).pauseMs);
}

void test_a_press_wakes_the_panel_without_also_changing_mode(void) {
    Harness h;
    h.begin(/*sleepTimeoutMs=*/5000);
    h.advance(6000);
    TEST_ASSERT_TRUE(h.panel.isAsleep());
    h.send(ButtonEvent::Short);
    TEST_ASSERT_FALSE(h.panel.isAsleep());
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Wind);
    h.send(ButtonEvent::Short);
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Signal);
}

void test_zero_timeout_never_sleeps(void) {
    Harness h;
    h.begin(/*sleepTimeoutMs=*/0);
    h.advance(600000);
    TEST_ASSERT_FALSE(h.panel.isAsleep());
}

void test_long_press_toggles_hard_off(void) {
    Harness h;
    h.begin();
    h.advance(2000);
    TEST_ASSERT_TRUE(litCount(h.out, h.now) > 0 || statusLanesInUse(h.out) > 0);
    h.send(ButtonEvent::Long);
    TEST_ASSERT_TRUE(h.panel.isHardOff());
    TEST_ASSERT_EQUAL_INT(0, statusLanesInUse(h.out));
    TEST_ASSERT_EQUAL_INT(0, detailPositionsInUse(h.out));
    h.advance(5000);
    TEST_ASSERT_EQUAL_INT(0, litCount(h.out, h.now));
    h.send(ButtonEvent::Long);
    TEST_ASSERT_FALSE(h.panel.isHardOff());
    h.advance(2000);
    TEST_ASSERT_TRUE(statusLanesInUse(h.out) > 0);
}

void test_hard_off_ignores_short_presses(void) {
    Harness h;
    h.begin();
    h.advance(2000);
    h.send(ButtonEvent::Long);
    h.send(ButtonEvent::Short);
    TEST_ASSERT_TRUE(h.panel.isHardOff());
    TEST_ASSERT_TRUE(h.panel.mode() == DetailMode::Wind);
}

void test_leds_disabled_keeps_all_nine_dark_forever(void) {
    // config.txt's leds.enabled=false - the durable way to silence the panel,
    // and it must beat the self-test, the fault codes and the button alike.
    Harness h;
    h.begin(/*sleepTimeoutMs=*/300000, /*enabled=*/false);
    h.inputs.configLoaded = false;  // a fault that would otherwise blink
    for (int i = 0; i < 20; i++) {
        h.advance(500);
        TEST_ASSERT_EQUAL_INT(0, litCount(h.out, h.now));
        TEST_ASSERT_EQUAL_INT(0, statusLanesInUse(h.out));
        TEST_ASSERT_EQUAL_INT(0, detailPositionsInUse(h.out));
    }
    h.send(ButtonEvent::Short);
    TEST_ASSERT_EQUAL_INT(0, litCount(h.out, h.now));
    h.send(ButtonEvent::Long);
    TEST_ASSERT_EQUAL_INT(0, litCount(h.out, h.now));
}

void test_the_heartbeat_doubles_when_readings_are_buffered(void) {
    Harness h;
    h.begin();
    h.advance(2000);
    TEST_ASSERT_EQUAL_INT(1, h.out.status[kStatusBlue].pattern.flashes);
    h.inputs.bufferedCount = 7;
    h.advance(100);
    TEST_ASSERT_EQUAL_INT(2, h.out.status[kStatusBlue].pattern.flashes);
}

void test_the_data_lane_distinguishes_stored_from_merely_accepted(void) {
    Harness h;
    h.begin();
    h.advance(2000);
    TEST_ASSERT_TRUE(h.out.status[kStatusGreen].pattern == lanePatternSolid());

    h.inputs.lastInserted = 0;  // 2xx, stored nothing - the bug-031 shape
    h.advance(100);
    TEST_ASSERT_TRUE(h.out.status[kStatusGreen].pattern == lanePatternSlowBlink());

    h.inputs.lastSendOk = false;
    h.advance(100);
    TEST_ASSERT_TRUE(h.out.status[kStatusGreen].pattern == lanePatternOff());
}

void test_the_radio_lane_blinks_only_on_a_genuinely_weak_signal(void) {
    Harness h;
    h.begin();
    h.advance(2000);
    TEST_ASSERT_TRUE(h.out.status[kStatusYellow].pattern == lanePatternSolid());
    h.inputs.rssiDbm = -70;  // usable - a lane that blinks here trains people to ignore it
    h.advance(100);
    TEST_ASSERT_TRUE(h.out.status[kStatusYellow].pattern == lanePatternSolid());
    h.inputs.rssiDbm = -85;
    h.advance(100);
    TEST_ASSERT_TRUE(h.out.status[kStatusYellow].pattern == lanePatternSlowBlink());
    h.inputs.wifiAssociated = false;
    h.advance(100);
    TEST_ASSERT_TRUE(h.out.status[kStatusYellow].pattern == lanePatternOff());
}

void test_the_direction_code_borrows_yellow_and_gives_it_back(void) {
    // The one bounded exception to "nothing touches the status group": the
    // fault lane is never borrowed, and the borrow is brief and self-ending.
    Harness h;
    h.begin();
    h.inputs.windDirOctant = 3;  // JV -> four flashes
    h.advance(1400);             // just past the self-test, in wind mode
    TEST_ASSERT_TRUE(h.out.status[kStatusYellow].pattern.oneShot);
    TEST_ASSERT_EQUAL_INT(4, h.out.status[kStatusYellow].pattern.flashes);
    TEST_ASSERT_EQUAL_INT(0, detailPositionsInUse(h.out));  // the dot steps aside
    TEST_ASSERT_TRUE(h.out.status[kStatusRed].pattern == lanePatternOff());

    h.advance(2000);  // the code is 4 * 300 ms; the next one is 5 s away
    TEST_ASSERT_FALSE(h.out.status[kStatusYellow].pattern.oneShot);
    TEST_ASSERT_EQUAL_INT(1, detailPositionsInUse(h.out));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_boot_self_test_lights_everything_then_sweeps_the_row);
    RUN_TEST(test_wind_is_the_default_mode);
    RUN_TEST(test_short_press_cycles_the_detail_modes);
    RUN_TEST(test_mode_banner_is_confined_to_the_detail_group);
    RUN_TEST(test_the_status_group_is_identical_in_every_detail_mode);
    RUN_TEST(test_a_fault_stays_visible_while_someone_looks_at_the_wind);
    RUN_TEST(test_exactly_one_detail_position_is_used_by_wind_and_signal);
    RUN_TEST(test_sensor_mode_uses_positions_positionally);
    RUN_TEST(test_anemometer_pulses_flash_position_one_live);
    RUN_TEST(test_a_counter_reset_is_a_resync_not_a_pulse);
    RUN_TEST(test_a_non_default_mode_returns_to_wind_by_itself);
    RUN_TEST(test_the_panel_sleeps_to_a_heartbeat_and_a_fault_code);
    RUN_TEST(test_a_press_wakes_the_panel_without_also_changing_mode);
    RUN_TEST(test_zero_timeout_never_sleeps);
    RUN_TEST(test_long_press_toggles_hard_off);
    RUN_TEST(test_hard_off_ignores_short_presses);
    RUN_TEST(test_leds_disabled_keeps_all_nine_dark_forever);
    RUN_TEST(test_the_heartbeat_doubles_when_readings_are_buffered);
    RUN_TEST(test_the_data_lane_distinguishes_stored_from_merely_accepted);
    RUN_TEST(test_the_radio_lane_blinks_only_on_a_genuinely_weak_signal);
    RUN_TEST(test_the_direction_code_borrows_yellow_and_gives_it_back);
    return UNITY_END();
}
