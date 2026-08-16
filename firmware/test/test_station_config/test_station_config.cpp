#include <unity.h>
#include "config/station_config.h"

void setUp(void) {}
void tearDown(void) {}

void test_parses_two_networks_backend_url_and_token(void) {
    const char *contents =
        "network1.ssid=ShoreWifi\n"
        "network1.password=shorepass\n"
        "network2.ssid=MobileHotspot\n"
        "network2.password=hotspotpass\n"
        "backend.url=http://192.168.1.50:3000\n"
        "backend.token=abc123\n";

    StationConfig config = parseStationConfig(contents);

    TEST_ASSERT_EQUAL_UINT(2, config.networks.size());
    TEST_ASSERT_EQUAL_STRING("ShoreWifi", config.networks[0].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("shorepass", config.networks[0].password.c_str());
    TEST_ASSERT_EQUAL_STRING("MobileHotspot", config.networks[1].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("hotspotpass", config.networks[1].password.c_str());
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.50:3000", config.backendBaseUrl.c_str());
    TEST_ASSERT_EQUAL_STRING("abc123", config.ingestToken.c_str());
}

void test_parses_a_single_network(void) {
    const char *contents =
        "network1.ssid=OnlyOne\n"
        "network1.password=onlypass\n"
        "backend.url=http://host\n"
        "backend.token=t\n";

    StationConfig config = parseStationConfig(contents);
    TEST_ASSERT_EQUAL_UINT(1, config.networks.size());
}

void test_ignores_blank_lines_and_comments(void) {
    const char *contents =
        "# Gustik station config\n"
        "\n"
        "network1.ssid=ShoreWifi\n"
        "network1.password=shorepass\n"
        "\n"
        "# backend settings\n"
        "backend.url=http://host\n"
        "backend.token=t\n";

    StationConfig config = parseStationConfig(contents);
    TEST_ASSERT_EQUAL_UINT(1, config.networks.size());
    TEST_ASSERT_EQUAL_STRING("http://host", config.backendBaseUrl.c_str());
}

void test_empty_contents_yields_empty_config_not_a_crash(void) {
    StationConfig config = parseStationConfig("");
    TEST_ASSERT_EQUAL_UINT(0, config.networks.size());
    TEST_ASSERT_EQUAL_STRING("", config.backendBaseUrl.c_str());
    TEST_ASSERT_EQUAL_STRING("", config.ingestToken.c_str());
}

void test_select_network_picks_highest_priority_network_that_is_in_range(void) {
    std::vector<WifiNetwork> configured = {
        {"ShoreWifi", "shorepass"},
        {"MobileHotspot", "hotspotpass"},
    };
    std::vector<std::string> availableSsids = {"SomeoneElsesWifi", "ShoreWifi", "MobileHotspot"};

    int index = selectNetworkIndex(configured, availableSsids);
    TEST_ASSERT_EQUAL_INT(0, index);
}

void test_select_network_falls_back_to_lower_priority_when_first_not_in_range(void) {
    std::vector<WifiNetwork> configured = {
        {"ShoreWifi", "shorepass"},
        {"MobileHotspot", "hotspotpass"},
    };
    std::vector<std::string> availableSsids = {"MobileHotspot"};

    int index = selectNetworkIndex(configured, availableSsids);
    TEST_ASSERT_EQUAL_INT(1, index);
}

void test_select_network_returns_negative_one_when_none_in_range(void) {
    std::vector<WifiNetwork> configured = {{"ShoreWifi", "shorepass"}};
    std::vector<std::string> availableSsids = {"SomeoneElsesWifi"};

    int index = selectNetworkIndex(configured, availableSsids);
    TEST_ASSERT_EQUAL_INT(-1, index);
}

void test_parses_magnetometer_hard_iron_offsets(void) {
    const char *contents =
        "backend.url=http://host\n"
        "mag.offsetX=1713.5\n"
        "mag.offsetY=-1984.0\n";

    StationConfig config = parseStationConfig(contents);

    TEST_ASSERT_TRUE(config.magnetometer.present);
    TEST_ASSERT_EQUAL_DOUBLE(1713.5, config.magnetometer.offsetX);
    TEST_ASSERT_EQUAL_DOUBLE(-1984.0, config.magnetometer.offsetY);
}

void test_magnetometer_absent_when_config_says_nothing(void) {
    const char *contents = "network1.ssid=OnlyOne\nbackend.url=http://host\n";

    StationConfig config = parseStationConfig(contents);

    TEST_ASSERT_FALSE(config.magnetometer.present);
}

// Half a hard-iron correction is worse than none - it rotates the heading by
// an arbitrary amount rather than leaving it uncorrected - so one key alone
// must not be applied.
void test_magnetometer_absent_when_only_one_axis_given(void) {
    StationConfig onlyX = parseStationConfig("mag.offsetX=1713.5\n");
    StationConfig onlyY = parseStationConfig("mag.offsetY=1984.0\n");

    TEST_ASSERT_FALSE(onlyX.magnetometer.present);
    TEST_ASSERT_FALSE(onlyY.magnetometer.present);
}

// A typo'd offset that silently parses as a plausible number would produce a
// confident, stable, wrong heading with no error anywhere.
void test_magnetometer_absent_when_value_is_not_a_number(void) {
    StationConfig config = parseStationConfig("mag.offsetX=1713.5abc\nmag.offsetY=1984.0\n");

    TEST_ASSERT_FALSE(config.magnetometer.present);
}

void test_magnetometer_accepts_negative_and_integer_forms(void) {
    StationConfig config = parseStationConfig("mag.offsetX=-42\nmag.offsetY=0\n");

    TEST_ASSERT_TRUE(config.magnetometer.present);
    TEST_ASSERT_EQUAL_DOUBLE(-42.0, config.magnetometer.offsetX);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, config.magnetometer.offsetY);
}

void test_led_panel_defaults_when_config_says_nothing(void) {
    StationConfig config = parseStationConfig("network1.ssid=x\n");

    TEST_ASSERT_TRUE(config.leds.enabled);
    TEST_ASSERT_EQUAL_UINT32(300, config.leds.timeoutSeconds);
}

void test_led_panel_can_be_disabled_without_a_reflash(void) {
    StationConfig config = parseStationConfig("leds.enabled=false\nleds.timeoutSeconds=0\n");

    TEST_ASSERT_FALSE(config.leds.enabled);
    TEST_ASSERT_EQUAL_UINT32(0, config.leds.timeoutSeconds);
}

void test_led_panel_accepts_the_spellings_people_actually_write(void) {
    TEST_ASSERT_FALSE(parseStationConfig("leds.enabled=0\n").leds.enabled);
    TEST_ASSERT_FALSE(parseStationConfig("leds.enabled=no\n").leds.enabled);
    TEST_ASSERT_FALSE(parseStationConfig("leds.enabled=off\n").leds.enabled);
    TEST_ASSERT_TRUE(parseStationConfig("leds.enabled=1\n").leds.enabled);
    TEST_ASSERT_TRUE(parseStationConfig("leds.enabled=yes\n").leds.enabled);
    TEST_ASSERT_TRUE(parseStationConfig("leds.enabled=on\n").leds.enabled);
}

void test_malformed_led_values_fall_back_to_defaults_not_garbage(void) {
    // The panel is a diagnostic. One that misbehaves because of a typo in the
    // file it exists to help you debug is worse than useless.
    StationConfig config = parseStationConfig("leds.enabled=maybe\nleds.timeoutSeconds=300x\n");

    TEST_ASSERT_TRUE(config.leds.enabled);
    TEST_ASSERT_EQUAL_UINT32(300, config.leds.timeoutSeconds);

    StationConfig negative = parseStationConfig("leds.timeoutSeconds=-5\n");
    TEST_ASSERT_EQUAL_UINT32(300, negative.leds.timeoutSeconds);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_led_panel_defaults_when_config_says_nothing);
    RUN_TEST(test_led_panel_can_be_disabled_without_a_reflash);
    RUN_TEST(test_led_panel_accepts_the_spellings_people_actually_write);
    RUN_TEST(test_malformed_led_values_fall_back_to_defaults_not_garbage);
    RUN_TEST(test_parses_two_networks_backend_url_and_token);
    RUN_TEST(test_parses_magnetometer_hard_iron_offsets);
    RUN_TEST(test_magnetometer_absent_when_config_says_nothing);
    RUN_TEST(test_magnetometer_absent_when_only_one_axis_given);
    RUN_TEST(test_magnetometer_absent_when_value_is_not_a_number);
    RUN_TEST(test_magnetometer_accepts_negative_and_integer_forms);
    RUN_TEST(test_parses_a_single_network);
    RUN_TEST(test_ignores_blank_lines_and_comments);
    RUN_TEST(test_empty_contents_yields_empty_config_not_a_crash);
    RUN_TEST(test_select_network_picks_highest_priority_network_that_is_in_range);
    RUN_TEST(test_select_network_falls_back_to_lower_priority_when_first_not_in_range);
    RUN_TEST(test_select_network_returns_negative_one_when_none_in_range);
    return UNITY_END();
}
