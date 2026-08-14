#pragma once

#include <Arduino.h>
#include "transmit/reading.h"
#include "transmit/ingest_response.h"
#include "config/station_config.h"

// What a POST /readings attempt actually achieved.
//
// A bare bool was not enough: a 2xx says the request arrived, not that
// anything was stored. The backend's `client_id` UNIQUE + ON CONFLICT DO
// NOTHING means a whole batch can be accepted and discarded, which is how
// bug-031 hid for hours behind a cheerful `sent=yes`. `response` carries the
// backend's own {received, inserted, duplicates, backfilled} counts when the
// body had them; see transmit/ingest_response.h.
struct SendResult {
    bool ok = false;      // true iff the HTTP status was 2xx
    int statusCode = 0;   // <=0 when no response was received at all
    IngestResponse response;

    // Lets `if (client.send(...))` keep reading naturally. Explicit, so a
    // caller that wants the counts has to name the struct rather than
    // silently collapsing it back to a bool.
    explicit operator bool() const { return ok; }
};

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
    // a result whose `ok` is false (never throws/crashes) on any WiFi or
    // HTTP failure, plus the backend's own stored-vs-received counts when
    // the response carried them. If WiFi is not currently connected,
    // attempts a (non-blocking-at-the-caller, best-effort) reconnect first
    // (AC2/AC3).
    SendResult send(const Reading *readings, size_t count);

private:
    StationConfig config_;

    bool ensureWifiConnected();
};
