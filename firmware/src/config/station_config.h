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

// Optional magnetometer hard-iron offsets, in raw LSB at the 8G field range
// sense/magnetometer.cpp configures. Lives in config.txt rather than only in
// source because a calibration describes one rigid assembly: remounting the
// sensor then costs a filesystem upload (`pio run -t uploadfs`) instead of a
// rebuild and reflash.
//
// `present` is false when config.txt names neither key, and main.cpp keeps
// its compiled-in default. Both keys are required together - a config that
// sets only one is treated as absent, since a half-applied hard-iron
// correction is worse than none.
//
// offsetY is in the firmware's *already Y-negated* mount frame (up=-z,
// forward=+x), matching what magnetometer.cpp's readRawXY() returns and what
// correct/wind_direction.cpp subtracts - i.e. the negative of the raw Y
// offset in scripts/qmc5883p-calibration.json. gustik_scripts.mag_calibrate
// emits these lines with the flip already applied, so they paste in verbatim.
struct MagnetometerCalibrationConfig {
    bool present = false;
    double offsetX = 0.0;
    double offsetY = 0.0;
};

struct StationConfig {
    std::vector<WifiNetwork> networks; // priority order, index 0 = highest priority
    std::string backendBaseUrl;
    std::string ingestToken;
    MagnetometerCalibrationConfig magnetometer;
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
