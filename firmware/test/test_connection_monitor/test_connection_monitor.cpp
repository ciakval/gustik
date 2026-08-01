#include <unity.h>
#include "transmit/connection_monitor.h"

void setUp(void) {}
void tearDown(void) {}

void test_starts_unhealthy_until_first_successful_send(void) {
    ConnectionMonitor monitor;
    // no send attempted yet - nothing has proven connectivity
    TEST_ASSERT_FALSE(monitor.isHealthy());
}

void test_becomes_healthy_after_a_successful_send(void) {
    ConnectionMonitor monitor;
    monitor.recordSendSuccess();
    TEST_ASSERT_TRUE(monitor.isHealthy());
}

void test_becomes_unhealthy_after_a_send_failure(void) {
    ConnectionMonitor monitor;
    monitor.recordSendSuccess();
    monitor.recordSendFailure();
    TEST_ASSERT_FALSE(monitor.isHealthy());
}

void test_recovers_to_healthy_after_next_successful_send(void) {
    // AC3: once WiFi/backend recovers, the very next successful send must
    // clear the unhealthy state - no multi-attempt debounce required.
    ConnectionMonitor monitor;
    monitor.recordSendFailure();
    monitor.recordSendFailure();
    monitor.recordSendSuccess();
    TEST_ASSERT_TRUE(monitor.isHealthy());
}

void test_repeated_failures_do_not_throw_or_change_after_first(void) {
    // AC2: a failing send must never put the monitor (and by extension the
    // sampling loop that owns it) into a broken/crashed state, no matter
    // how many times it fails in a row.
    ConnectionMonitor monitor;
    for (int i = 0; i < 50; i++) {
        monitor.recordSendFailure();
    }
    TEST_ASSERT_FALSE(monitor.isHealthy());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_starts_unhealthy_until_first_successful_send);
    RUN_TEST(test_becomes_healthy_after_a_successful_send);
    RUN_TEST(test_becomes_unhealthy_after_a_send_failure);
    RUN_TEST(test_recovers_to_healthy_after_next_successful_send);
    RUN_TEST(test_repeated_failures_do_not_throw_or_change_after_first);
    return UNITY_END();
}
