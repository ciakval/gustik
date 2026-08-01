#include <unity.h>
#include <string>
#include "transmit/payload.h"

void setUp(void) {}
void tearDown(void) {}

Reading makeReading(const char *clientId, double speed, int octant, bool rssiValid, int rssi) {
    Reading r;
    r.clientId = clientId;
    r.capturedAt = "2026-08-01T09:00:00.000Z";
    r.clockSynced = true;
    r.windSpeedMs = speed;
    r.windDirOctant = octant;
    r.rssiValid = rssiValid;
    r.rssiDbm = rssi;
    return r;
}

void test_serializes_single_reading_as_array_of_one(void) {
    Reading r = makeReading("c-1", 3.5, 2, true, -55);
    std::string json = buildReadingsPayloadJson(&r, 1);

    std::string expected =
        "[{\"clientId\":\"c-1\","
        "\"capturedAt\":\"2026-08-01T09:00:00.000Z\","
        "\"clockSynced\":true,"
        "\"windSpeedMs\":3.5,"
        "\"windDirOctant\":2,"
        "\"rssiDbm\":-55}]";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), json.c_str());
}

void test_serializes_multiple_readings_for_backfill_shape(void) {
    Reading readings[2] = {
        makeReading("c-1", 1.0, 0, true, -60),
        makeReading("c-2", 2.0, 1, true, -61),
    };
    std::string json = buildReadingsPayloadJson(readings, 2);

    TEST_ASSERT_TRUE(json.front() == '[');
    TEST_ASSERT_TRUE(json.back() == ']');
    // two objects present, comma-separated
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"clientId\":\"c-1\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"clientId\":\"c-2\""));
}

void test_null_rssi_when_not_yet_scanned(void) {
    Reading r = makeReading("c-1", 1.0, 0, /*rssiValid=*/false, 0);
    std::string json = buildReadingsPayloadJson(&r, 1);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"rssiDbm\":null"));
}

void test_empty_array_for_zero_readings(void) {
    std::string json = buildReadingsPayloadJson(nullptr, 0);
    TEST_ASSERT_EQUAL_STRING("[]", json.c_str());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_serializes_single_reading_as_array_of_one);
    RUN_TEST(test_serializes_multiple_readings_for_backfill_shape);
    RUN_TEST(test_null_rssi_when_not_yet_scanned);
    RUN_TEST(test_empty_array_for_zero_readings);
    return UNITY_END();
}
