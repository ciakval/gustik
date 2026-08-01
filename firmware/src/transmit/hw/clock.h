#pragma once

#include <Arduino.h>

// SNTP-backed wall clock (AD-2: Stanice synchronizes via SNTP at boot and
// after every reconnect; captured_at falls back to a receivedAt-based
// estimate on the backend when never synced - see clockSynced on Reading).
// Hardware-coupled (uses ESP32 time.h/configTime) - not unit tested.
class StationClock {
public:
    void begin();
    // Call after a WiFi reconnect (AD-2) to retry sync if it never succeeded.
    void resyncIfNeeded();
    bool isSynced() const;
    // Current time as an ISO-8601 UTC string, e.g. "2026-08-01T09:00:00.000Z".
    // Meaningful only when isSynced() is true - callers must check that and
    // set the record's clockSynced flag accordingly (AD-2), never assume.
    String nowIso8601() const;

private:
    bool synced_ = false;
};
