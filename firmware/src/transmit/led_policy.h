#pragma once

// Pure decision for the disconnect LED (Story 2.4, FR-5/NFR-2): on whenever
// the connection is unhealthy, off as soon as it's healthy again. Turning
// it on immediately (no debounce) trivially satisfies the <=10s reaction
// budget - deliberate, since debouncing a safety-relevant signal to "wait
// and see" would work against its purpose.
inline bool shouldLedSignalDisconnect(bool connectionHealthy) {
    return !connectionHealthy;
}
