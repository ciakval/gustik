#pragma once

#include <cstddef>

// Computes how many buffer slots are needed to cover at least
// `minHours` of data at the given sampling interval (NFR-4: local buffer
// capacity >= 4h at the usual sampling frequency).
size_t computeBufferCapacityForHours(double minHours, double sampleIntervalSeconds);
