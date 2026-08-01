#include "transmit/hw/wifi_client.h"
#include "transmit/payload.h"
#include <WiFi.h>
#include <HTTPClient.h>

namespace {
constexpr unsigned long kWifiConnectTimeoutMs = 5000;
}

void WifiTransmitClient::begin(const char *ssid, const char *password, const char *backendBaseUrl, const char *ingestToken) {
    ssid_ = ssid;
    password_ = password;
    backendBaseUrl_ = backendBaseUrl;
    ingestToken_ = ingestToken;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_.c_str(), password_.c_str());
}

bool WifiTransmitClient::ensureWifiConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    // Best-effort, bounded reconnect attempt - never blocks the sampling
    // loop indefinitely (AC2: no restart/operator intervention needed, but
    // also no unbounded blocking on a dead network).
    WiFi.disconnect();
    WiFi.begin(ssid_.c_str(), password_.c_str());
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
    http.begin(backendBaseUrl_ + "/readings");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + ingestToken_);

    String body = buildReadingsPayloadJson(readings, count).c_str();
    int statusCode = http.POST(body);
    http.end();

    return statusCode >= 200 && statusCode < 300;
}
