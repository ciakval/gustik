#include <unity.h>
#include "transmit/rssi_latch.h"

void setUp(void) {}
void tearDown(void) {}

void test_starts_unavailable_before_any_scan(void) {
    // AC2: no rssiDbm before the first successful WiFi scan after boot.
    RssiAvailabilityLatch latch;
    TEST_ASSERT_FALSE(latch.isAvailable());
}

void test_becomes_available_after_first_connected_scan(void) {
    RssiAvailabilityLatch latch;
    latch.recordScanResult(/*wifiConnectedThisCycle=*/true);
    TEST_ASSERT_TRUE(latch.isAvailable());
}

void test_stays_available_once_latched_even_if_later_disconnected(void) {
    // AC2's condition is "since boot", not "currently connected" - a
    // temporary disconnect after a successful first scan must not make
    // rssiDbm start reporting NULL again.
    RssiAvailabilityLatch latch;
    latch.recordScanResult(true);
    latch.recordScanResult(false);
    latch.recordScanResult(false);
    TEST_ASSERT_TRUE(latch.isAvailable());
}

void test_stays_unavailable_across_repeated_disconnected_cycles(void) {
    RssiAvailabilityLatch latch;
    for (int i = 0; i < 10; i++) {
        latch.recordScanResult(false);
    }
    TEST_ASSERT_FALSE(latch.isAvailable());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_starts_unavailable_before_any_scan);
    RUN_TEST(test_becomes_available_after_first_connected_scan);
    RUN_TEST(test_stays_available_once_latched_even_if_later_disconnected);
    RUN_TEST(test_stays_unavailable_across_repeated_disconnected_cycles);
    return UNITY_END();
}
