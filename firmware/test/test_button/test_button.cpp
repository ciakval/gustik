#include <unity.h>
#include "indicate/button.h"

void setUp(void) {}
void tearDown(void) {}

namespace {

// Feeds a level to the decoder every millisecond from `fromMs` to `toMs`,
// returning the last non-None event seen (there is at most one per gesture).
ButtonEvent hold(ButtonDecoder &decoder, bool pressed, unsigned long fromMs, unsigned long toMs) {
    ButtonEvent seen = ButtonEvent::None;
    for (unsigned long t = fromMs; t <= toMs; t++) {
        ButtonEvent event = decoder.update(pressed, t);
        if (event != ButtonEvent::None) {
            seen = event;
        }
    }
    return seen;
}

// A complete press-and-release of `heldMs`, starting at `startMs`. Returns
// the event delivered, and leaves the decoder settled in "released".
ButtonEvent press(ButtonDecoder &decoder, unsigned long startMs, unsigned long heldMs) {
    hold(decoder, false, startMs, startMs + 50);
    unsigned long down = startMs + 51;
    hold(decoder, true, down, down + heldMs - 1);
    return hold(decoder, false, down + heldMs, down + heldMs + 100);
}

} // namespace

void test_no_button_fitted_emits_nothing(void) {
    // INPUT_PULLUP with nothing attached reads "released" forever. This is
    // the whole of "absent hardware is a no-op" for the button, and it is a
    // tested invariant rather than an assumption.
    ButtonDecoder decoder;
    TEST_ASSERT_TRUE(hold(decoder, false, 0, 60000) == ButtonEvent::None);
}

void test_short_press_advances(void) {
    ButtonDecoder decoder;
    TEST_ASSERT_TRUE(press(decoder, 1000, 200) == ButtonEvent::Short);
}

void test_long_press_is_long_and_never_also_short(void) {
    ButtonDecoder decoder;
    // A long press passes through the short-press duration on its way, so
    // "acts on release" is what keeps hard-off from also cycling the mode.
    hold(decoder, false, 0, 50);
    ButtonEvent duringHold = hold(decoder, true, 51, 51 + 2500);
    TEST_ASSERT_TRUE(duringHold == ButtonEvent::None);
    ButtonEvent onRelease = hold(decoder, false, 2552, 2652);
    TEST_ASSERT_TRUE(onRelease == ButtonEvent::Long);
}

void test_contact_bounce_is_swallowed(void) {
    ButtonDecoder decoder;
    hold(decoder, false, 0, 100);
    // 8 ms of chatter, then a real 200 ms press.
    unsigned long t = 101;
    for (int i = 0; i < 4; i++) {
        decoder.update(true, t++);
        decoder.update(false, t++);
    }
    ButtonEvent duringChatter = hold(decoder, false, t, t + 5);
    TEST_ASSERT_TRUE(duringChatter == ButtonEvent::None);
    hold(decoder, true, t + 6, t + 206);
    TEST_ASSERT_TRUE(hold(decoder, false, t + 207, t + 307) == ButtonEvent::Short);
}

void test_a_blip_shorter_than_the_debounce_window_is_not_a_press(void) {
    ButtonDecoder decoder;
    hold(decoder, false, 0, 100);
    hold(decoder, true, 101, 101 + 20);  // 20 ms, under the 30 ms window
    TEST_ASSERT_TRUE(hold(decoder, false, 122, 300) == ButtonEvent::None);
}

void test_the_dead_zone_between_short_and_long_does_nothing(void) {
    // Releasing in 800 ms - 2 s is an ambiguous gesture, and doing nothing is
    // the safe answer: the alternative is toggling hard off by accident.
    ButtonDecoder decoder;
    TEST_ASSERT_TRUE(press(decoder, 0, 1200) == ButtonEvent::None);
}

void test_boundaries(void) {
    ButtonDecoder shortEdge;
    TEST_ASSERT_TRUE(press(shortEdge, 0, kButtonShortMaxMs) == ButtonEvent::Short);
    ButtonDecoder justOver;
    TEST_ASSERT_TRUE(press(justOver, 0, kButtonShortMaxMs + 1) == ButtonEvent::None);
    ButtonDecoder longEdge;
    TEST_ASSERT_TRUE(press(longEdge, 0, kButtonLongMs) == ButtonEvent::Long);
}

void test_button_held_through_reset_does_not_fire_on_release(void) {
    // GPIO13 is not a strapping pin, so holding the button at boot is safe -
    // but the decoder must adopt the level rather than treat the release as
    // a gesture nobody made.
    ButtonDecoder decoder;
    hold(decoder, true, 0, 500);
    TEST_ASSERT_TRUE(hold(decoder, false, 501, 700) == ButtonEvent::None);
}

void test_repeated_presses_each_deliver_one_event(void) {
    ButtonDecoder decoder;
    TEST_ASSERT_TRUE(press(decoder, 0, 100) == ButtonEvent::Short);
    TEST_ASSERT_TRUE(press(decoder, 2000, 100) == ButtonEvent::Short);
    TEST_ASSERT_TRUE(press(decoder, 4000, 2500) == ButtonEvent::Long);
    TEST_ASSERT_TRUE(press(decoder, 9000, 100) == ButtonEvent::Short);
}

void test_long_press_pending_reports_the_hold_before_release(void) {
    ButtonDecoder decoder;
    hold(decoder, false, 0, 50);
    hold(decoder, true, 51, 51 + 500);
    TEST_ASSERT_FALSE(decoder.longPressPending(551));
    hold(decoder, true, 552, 51 + 2100);
    TEST_ASSERT_TRUE(decoder.longPressPending(51 + 2100));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_no_button_fitted_emits_nothing);
    RUN_TEST(test_short_press_advances);
    RUN_TEST(test_long_press_is_long_and_never_also_short);
    RUN_TEST(test_contact_bounce_is_swallowed);
    RUN_TEST(test_a_blip_shorter_than_the_debounce_window_is_not_a_press);
    RUN_TEST(test_the_dead_zone_between_short_and_long_does_nothing);
    RUN_TEST(test_boundaries);
    RUN_TEST(test_button_held_through_reset_does_not_fire_on_release);
    RUN_TEST(test_repeated_presses_each_deliver_one_event);
    RUN_TEST(test_long_press_pending_reports_the_hold_before_release);
    return UNITY_END();
}
