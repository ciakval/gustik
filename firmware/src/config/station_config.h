#pragma once

#include <string>
#include <vector>

// Wi-Fi credentials + ingest token live in this config file's data (AD-10:
// "on flash, never compiled into firmware source"). The file itself
// (firmware/data/config.example.txt, a template) is uploaded to the
// device's LittleFS partition separately from the compiled program via
// PlatformIO's filesystem upload target (`pio run -t uploadfs`) - a
// distinct, faster operation than reflashing the program (Story 4.1 AC3:
// a config change is a serial/USB filesystem re-upload, not a full reflash).
struct WifiNetwork {
    std::string ssid;
    std::string password;
};

struct StationConfig {
    std::vector<WifiNetwork> networks; // priority order, index 0 = highest priority
    std::string backendBaseUrl;
    std::string ingestToken;
};

// Parses the simple `key=value` config file format (one entry per line,
// blank lines and `#`-prefixed comments ignored). Pure - no filesystem
// access, so it's unit testable without LittleFS/hardware. Never throws:
// malformed/empty input yields an empty-ish StationConfig, not a crash -
// consistent with firmware's "never block/crash" rule elsewhere.
StationConfig parseStationConfig(const std::string &fileContents);

// Picks the highest-priority configured network that is actually in scan
// range (Story 4.1 AC2). Returns the index into `configured`, or -1 if none
// of the configured networks are currently in range.
int selectNetworkIndex(const std::vector<WifiNetwork> &configured, const std::vector<std::string> &availableSsids);
