#pragma once

#include "config/station_config.h"

// Reads /config.txt from LittleFS and parses it (config/station_config.h).
// Hardware-coupled (LittleFS), not unit tested. Returns an empty
// StationConfig (zero networks) if the file is missing/unreadable - the
// firmware never blocks/crashes on a missing config, it just can't connect
// to WiFi until one is uploaded (visible via the disconnect LED, Story 2.4).
StationConfig loadStationConfig();
