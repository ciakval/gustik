#include "transmit/hw/wifi_client.h"
#include "transmit/payload.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <algorithm>
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

// Best-effort, bounded connect attempt against one network - never blocks
// the sampling loop indefinitely (AC2: no restart/operator intervention
// needed, but also no unbounded blocking on a dead network).
bool tryConnect(const WifiNetwork &network) {
    WiFi.disconnect();
    WiFi.begin(network.ssid.c_str(), network.password.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < kWifiConnectTimeoutMs) {
        delay(100);
    }
    return WiFi.status() == WL_CONNECTED;
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
    // in range, in priority order - unattended, no operator input. Try
    // every in-range configured network in turn (not just the top
    // priority one) so a network that's visible in the scan but won't
    // actually connect (wrong password, AP rejecting the station) doesn't
    // permanently block falling back to the other configured network.
    std::vector<std::string> availableSsids = scanAvailableSsids();
    for (size_t attempt = 0; attempt < config_.networks.size(); attempt++) {
        int chosen = selectNetworkIndex(config_.networks, availableSsids);
        if (chosen < 0) {
            break; // none of the remaining configured networks are in range
        }

        const WifiNetwork &network = config_.networks[chosen];
        if (tryConnect(network)) {
            return true;
        }

        // Drop this SSID from the candidate pool so the next loop
        // iteration's selectNetworkIndex falls through to the next
        // priority network instead of retrying the one that just failed.
        availableSsids.erase(std::remove(availableSsids.begin(), availableSsids.end(), network.ssid), availableSsids.end());
    }
    return false;
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
