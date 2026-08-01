#include <unity.h>
#include "transmit/led_policy.h"

void setUp(void) {}
void tearDown(void) {}

void test_led_on_when_connection_unhealthy(void) {
    // AC1 (NFR-2): turning the LED on immediately on detected failure is
    // well within the <=10s reaction budget - no debounce for a
    // safety-relevant signal.
    TEST_ASSERT_TRUE(shouldLedSignalDisconnect(/*connectionHealthy=*/false));
}

void test_led_off_when_connection_healthy(void) {
    // AC2: LED turns off as soon as the connection is healthy again -
    // which ConnectionMonitor already makes true on the very next
    // successful send (see test_connection_monitor's recovery test).
    TEST_ASSERT_FALSE(shouldLedSignalDisconnect(/*connectionHealthy=*/true));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_led_on_when_connection_unhealthy);
    RUN_TEST(test_led_off_when_connection_healthy);
    return UNITY_END();
}
