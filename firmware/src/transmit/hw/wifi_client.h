#pragma once

#include <Arduino.h>
#include "transmit/reading.h"
#include "config/station_config.h"

// WiFi + HTTP POST /readings client. Hardware-coupled (WiFi.h,
// HTTPClient.h) - not part of the native test build; see
// transmit/payload.h, transmit/connection_monitor.h and
// config/station_config.h (selectNetworkIndex) for the testable logic
// this wraps.
//
// send() never blocks indefinitely or crashes on failure (FR-4's
// requirement that firmware never blocks on a send error) - it returns
// false and the sampling loop continues regardless (AC2). Reconnection is
// attempted opportunistically on the next call if WiFi is down.
class WifiTransmitClient {
public:
    // Story 4.1: takes the full StationConfig (1-2 candidate networks in
    // priority order) rather than a single SSID/password, since the
    // Station must connect unattended to whichever configured network is
    // actually in range (AC2).
    void begin(const StationConfig &config);

    // Sends the given readings as a single POST /readings request. Returns
    // false (never throws/crashes) on any WiFi or HTTP failure. If WiFi is
    // not currently connected, attempts a (non-blocking-at-the-caller,
    // best-effort) reconnect first (AC2/AC3).
    bool send(const Reading *readings, size_t count);

private:
    StationConfig config_;

    bool ensureWifiConnected();
};
