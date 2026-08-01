#pragma once

#include <Arduino.h>
#include "transmit/reading.h"

// WiFi + HTTP POST /readings client. Hardware-coupled (WiFi.h,
// HTTPClient.h) - not part of the native test build; see
// transmit/payload.h and transmit/connection_monitor.h for the testable
// logic this wraps.
//
// send() never blocks indefinitely or crashes on failure (FR-4's
// requirement that firmware never blocks on a send error) - it returns
// false and the sampling loop continues regardless (AC2). Reconnection is
// attempted opportunistically on the next call if WiFi is down.
class WifiTransmitClient {
public:
    void begin(const char *ssid, const char *password, const char *backendBaseUrl, const char *ingestToken);

    // Sends the given readings as a single POST /readings request. Returns
    // false (never throws/crashes) on any WiFi or HTTP failure. If WiFi is
    // not currently connected, attempts a (non-blocking-at-the-caller,
    // best-effort) reconnect first (AC2/AC3).
    bool send(const Reading *readings, size_t count);

private:
    String ssid_;
    String password_;
    String backendBaseUrl_;
    String ingestToken_;

    bool ensureWifiConnected();
};
