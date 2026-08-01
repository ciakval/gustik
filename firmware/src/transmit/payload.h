#pragma once

#include <string>
#include "transmit/reading.h"

// Serializes readings into the JSON array body POST /readings expects
// (AD-8: array of 1..N, camelCase field names matching backend/src/store).
// Pure string building - no Arduino/network dependency, so it's unit
// testable without hardware.
std::string buildReadingsPayloadJson(const Reading *readings, size_t count);
