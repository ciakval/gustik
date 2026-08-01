#include "transmit/hw/wifi_client.h"
#include "transmit/payload.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <vector>
#include <string>

namespace {
constexpr unsigned long kWifiConnectTimeoutMs = 5000;

// Scans for nearby networks and returns their SSIDs. Bounded/best-effort -
// WiFi.scanNetworks() blocks until done or a timeout internally; we just
// don't retry indefinitely here either way.
std::vector<std::string> scanAvailableSsids() {
    std::vector<std::string> ssids;
    int count = WiFi.scanNetworks();
    for (int i = 0; i < count; i++) {
        ssids.push_back(std::string(WiFi.SSID(i).c_str()));
    }
    return ssids;
}
} // namespace

void WifiTransmitClient::begin(const StationConfig &config) {
    config_ = config;
    WiFi.mode(WIFI_STA);
}

bool WifiTransmitClient::ensureWifiConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }
    if (config_.networks.empty()) {
        return false; // no config uploaded yet - nothing to connect to
    }

    // Story 4.1 AC2: connect to whichever configured network is actually
    // in range, in priority order - unattended, no operator input.
    std::vector<std::string> availableSsids = scanAvailableSsids();
    int chosen = selectNetworkIndex(config_.networks, availableSsids);
    if (chosen < 0) {
        return false; // none of the configured networks are in range right now
    }

    const WifiNetwork &network = config_.networks[chosen];
    WiFi.disconnect();
    WiFi.begin(network.ssid.c_str(), network.password.c_str());

    // Best-effort, bounded reconnect attempt - never blocks the sampling
    // loop indefinitely (AC2: no restart/operator intervention needed, but
    // also no unbounded blocking on a dead network).
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < kWifiConnectTimeoutMs) {
        delay(100);
    }
    return WiFi.status() == WL_CONNECTED;
}

bool WifiTransmitClient::send(const Reading *readings, size_t count) {
    if (!ensureWifiConnected()) {
        return false;
    }

    HTTPClient http;
    http.begin(String(config_.backendBaseUrl.c_str()) + "/readings");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + config_.ingestToken.c_str());

    String body = buildReadingsPayloadJson(readings, count).c_str();
    int statusCode = http.POST(body);
    http.end();

    return statusCode >= 200 && statusCode < 300;
}
