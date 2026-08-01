#include "sense/vane.h"

namespace {
// TODO(calibration): placeholder ADC readings (12-bit, 0-4095) for the 8
// native vane positions - must be measured on the actual salvaged
// WH1080/WH1090 vane before deployment (its resistor ladder is specific to
// the unit), see TODO.md. Index = raw octant.
constexpr int kOctantAdcReadings[8] = {
    3200, 1600, 2200, 3800, 3950, 3450, 2800, 2400,
};
} // namespace

void Vane::begin(uint8_t analogPin) {
    pin_ = analogPin;
    pinMode(pin_, INPUT);
}

int Vane::readRawOctant() {
    int reading = analogRead(pin_);

    int nearestOctant = 0;
    int smallestDiff = abs(reading - kOctantAdcReadings[0]);
    for (int octant = 1; octant < 8; octant++) {
        int diff = abs(reading - kOctantAdcReadings[octant]);
        if (diff < smallestDiff) {
            smallestDiff = diff;
            nearestOctant = octant;
        }
    }
    return nearestOctant;
}
