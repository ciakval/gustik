#pragma once

// Tracks whether the last send attempt succeeded, independent of any actual
// WiFi/HTTP call - pure state, unit testable. The hardware transmit layer
// (transmit/hw/) calls recordSendSuccess()/recordSendFailure() after each
// real send attempt and uses isHealthy() to decide whether a WiFi reconnect
// is warranted (AC2/AC3) and, later, whether to signal the disconnect LED
// (Story 2.4).
class ConnectionMonitor {
public:
    void recordSendSuccess();
    void recordSendFailure();
    bool isHealthy() const;

private:
    bool healthy_ = false;
};
