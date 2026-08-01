#include "transmit/hw/clock.h"
#include <time.h>

namespace {
constexpr const char *kNtpServer = "pool.ntp.org";
constexpr unsigned long kSyncWaitTimeoutMs = 5000;
}

void StationClock::begin() {
    configTime(0, 0, kNtpServer); // UTC, no DST offset
    struct tm timeinfo;
    synced_ = getLocalTime(&timeinfo, kSyncWaitTimeoutMs);
}

void StationClock::resyncIfNeeded() {
    if (synced_) {
        return;
    }
    begin();
}

bool StationClock::isSynced() const {
    return synced_;
}

String StationClock::nowIso8601() const {
    time_t now;
    time(&now);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);

    char buf[25];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);
    return String(buf);
}
