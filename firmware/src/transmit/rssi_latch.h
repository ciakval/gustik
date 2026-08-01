#pragma once

// Tracks whether the Station has ever completed a successful WiFi scan
// since boot (Story 2.5 AC2/FR-6) - a one-shot latch, not a live
// "currently connected" flag, so RSSI keeps reporting the last known value
// through brief reconnects instead of flapping back to NULL.
class RssiAvailabilityLatch {
public:
    void recordScanResult(bool wifiConnectedThisCycle) {
        if (wifiConnectedThisCycle) {
            hasScannedOnce_ = true;
        }
    }

    bool isAvailable() const { return hasScannedOnce_; }

private:
    bool hasScannedOnce_ = false;
};
