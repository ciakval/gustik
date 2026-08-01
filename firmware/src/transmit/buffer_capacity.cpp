#include "transmit/buffer_capacity.h"
#include <cmath>

size_t computeBufferCapacityForHours(double minHours, double sampleIntervalSeconds) {
    double neededSlots = (minHours * 3600.0) / sampleIntervalSeconds;
    return static_cast<size_t>(std::ceil(neededSlots));
}
