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

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_parses_two_networks_backend_url_and_token);
    RUN_TEST(test_parses_a_single_network);
    RUN_TEST(test_ignores_blank_lines_and_comments);
    RUN_TEST(test_empty_contents_yields_empty_config_not_a_crash);
    RUN_TEST(test_select_network_picks_highest_priority_network_that_is_in_range);
    RUN_TEST(test_select_network_falls_back_to_lower_priority_when_first_not_in_range);
    RUN_TEST(test_select_network_returns_negative_one_when_none_in_range);
    return UNITY_END();
}
