#include <unity.h>
#include "indicate/fault.h"

void setUp(void) {}
void tearDown(void) {}

namespace {

// A station in perfect health: config parsed, associated, last POST stored a
// row, nothing buffered, clock synced, both sensors answering.
PanelInputs healthy() {
    PanelInputs in;
    in.haveSample = true;
    in.configLoaded = true;
    in.wifiAssociated = true;
    in.rssiValid = true;
    in.rssiDbm = -50;
    in.lastSendOk = true;
    in.lastHttpStatus = 201;
    in.hasCounts = true;
    in.lastInserted = 1;
    in.bufferedCount = 0;
    in.clockSynced = true;
    in.magnetometerOk = true;
    in.vaneInRange = true;
    return in;
}

} // namespace

void test_healthy_station_has_no_fault(void) {
    TEST_ASSERT_TRUE(highestPriorityFault(healthy()) == PanelFault::None);
    TEST_ASSERT_EQUAL_INT(0, faultFlashCount(PanelFault::None));
}

void test_every_code_is_reachable(void) {
    PanelInputs in;

    in = healthy();
    in.configLoaded = false;
    TEST_ASSERT_EQUAL_INT(1, faultFlashCount(highestPriorityFault(in)));

    in = healthy();
    in.wifiAssociated = false;
    TEST_ASSERT_EQUAL_INT(2, faultFlashCount(highestPriorityFault(in)));

    in = healthy();
    in.lastSendOk = false;
    in.lastHttpStatus = 0;  // nothing came back at all
    TEST_ASSERT_EQUAL_INT(3, faultFlashCount(highestPriorityFault(in)));

    in = healthy();
    in.lastSendOk = false;
    in.lastHttpStatus = 401;
    TEST_ASSERT_EQUAL_INT(4, faultFlashCount(highestPriorityFault(in)));

    in = healthy();
    in.lastInserted = 0;
    TEST_ASSERT_EQUAL_INT(5, faultFlashCount(highestPriorityFault(in)));

    in = healthy();
    in.bufferedCount = 12;
    TEST_ASSERT_EQUAL_INT(6, faultFlashCount(highestPriorityFault(in)));

    in = healthy();
    in.clockSynced = false;
    TEST_ASSERT_EQUAL_INT(7, faultFlashCount(highestPriorityFault(in)));

    in = healthy();
    in.magnetometerOk = false;
    TEST_ASSERT_EQUAL_INT(8, faultFlashCount(highestPriorityFault(in)));

    in = healthy();
    in.vaneInRange = false;
    TEST_ASSERT_EQUAL_INT(8, faultFlashCount(highestPriorityFault(in)));
}

void test_bug031_signature_is_code_5(void) {
    // HTTP 2xx, `inserted == 0`: the request succeeded and the backend
    // stored nothing. This printed `sent=yes` for hours before it was found.
    PanelInputs in = healthy();
    in.lastSendOk = true;
    in.lastHttpStatus = 200;
    in.hasCounts = true;
    in.lastInserted = 0;
    TEST_ASSERT_TRUE(highestPriorityFault(in) == PanelFault::StoredNothing);
}

void test_a_backend_without_counts_never_fabricates_code_5(void) {
    // An older backend answers {"written":1} and no counts. lastInserted is
    // then a default zero that reads exactly like the alarm above - so
    // hasCounts is required, the same guard transmit/ingest_response.h uses.
    PanelInputs in = healthy();
    in.hasCounts = false;
    in.lastInserted = 0;
    TEST_ASSERT_TRUE(highestPriorityFault(in) == PanelFault::None);
}

void test_lower_code_wins(void) {
    // Priority is the code number ascending, because the numbering is
    // ordered by causal depth: with no config, the WiFi state is meaningless.
    PanelInputs in = healthy();
    in.configLoaded = false;
    in.wifiAssociated = false;
    in.lastSendOk = false;
    in.lastHttpStatus = 401;
    in.bufferedCount = 99;
    in.clockSynced = false;
    in.magnetometerOk = false;
    TEST_ASSERT_TRUE(highestPriorityFault(in) == PanelFault::NoConfig);

    in.configLoaded = true;
    TEST_ASSERT_TRUE(highestPriorityFault(in) == PanelFault::NoNetwork);

    in.wifiAssociated = true;
    TEST_ASSERT_TRUE(highestPriorityFault(in) == PanelFault::TokenRejected);
}

void test_no_transmit_fault_before_the_first_sample(void) {
    // At boot lastSendOk is false and lastHttpStatus is 0, which would
    // otherwise read as "backend unreachable" for the first three seconds of
    // every boot - a fault code that is always wrong is worse than none.
    PanelInputs in;
    in.haveSample = false;
    in.configLoaded = true;
    in.wifiAssociated = true;
    TEST_ASSERT_TRUE(highestPriorityFault(in) == PanelFault::None);
}

void test_config_and_network_faults_apply_before_the_first_sample(void) {
    // These two are answerable from setup() onward, so they are not gated.
    PanelInputs in;
    in.haveSample = false;
    TEST_ASSERT_TRUE(highestPriorityFault(in) == PanelFault::NoConfig);
    in.configLoaded = true;
    TEST_ASSERT_TRUE(highestPriorityFault(in) == PanelFault::NoNetwork);
}

void test_fatal_outranks_everything_and_is_not_a_flash_code(void) {
    PanelInputs in = healthy();
    in.fatal = true;
    in.configLoaded = false;
    TEST_ASSERT_TRUE(highestPriorityFault(in) == PanelFault::Fatal);
    // Rendered solid, so it must not also be countable as a code.
    TEST_ASSERT_EQUAL_INT(0, faultFlashCount(PanelFault::Fatal));
}

void test_a_calm_anemometer_is_never_a_fault(void) {
    // No pulses is a legitimate reading of a calm day. Only a person
    // watching the cups turn can tell it from a broken wire (bug-059), which
    // is what sensor mode is for - it must not raise code 8 by itself.
    PanelInputs in = healthy();
    in.pulseCountSnapshot = 0;
    in.windSpeedMs = 0.0;
    TEST_ASSERT_TRUE(highestPriorityFault(in) == PanelFault::None);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_healthy_station_has_no_fault);
    RUN_TEST(test_every_code_is_reachable);
    RUN_TEST(test_bug031_signature_is_code_5);
    RUN_TEST(test_a_backend_without_counts_never_fabricates_code_5);
    RUN_TEST(test_lower_code_wins);
    RUN_TEST(test_no_transmit_fault_before_the_first_sample);
    RUN_TEST(test_config_and_network_faults_apply_before_the_first_sample);
    RUN_TEST(test_fatal_outranks_everything_and_is_not_a_flash_code);
    RUN_TEST(test_a_calm_anemometer_is_never_a_fault);
    return UNITY_END();
}
