#pragma once

#include <string>

// Wire-shape reading, mirrors backend's POST /readings input contract
// exactly (backend/src/store camelCase boundary). Shared by the live-send
// path (Story 1.4, arrays of 1) and backfill (Story 2.2, arrays of N).
struct Reading {
    std::string clientId;
    std::string capturedAt; // ISO-8601 UTC
    bool clockSynced;
    double windSpeedMs;
    int windDirOctant;
    bool rssiValid; // false before first successful WiFi scan (Story 2.5, FR-6)
    int rssiDbm;
};
