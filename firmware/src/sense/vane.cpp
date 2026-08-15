#include "sense/vane.h"

namespace {
// Measured 2026-08-15 on the real salvaged WH1080/WH1090 vane with the real
// 10kohm pull-up and 3.3V rail, using diag/vane_diag.cpp (two full hand
// rotations, 18-78 settled samples per octant, per-octant ADC spread 2-6
// counts). Cross-checked against an independent earlier capture: every
// octant agreed to within 1 count. Full method, resistances and caveats:
// docs/hardware/wind-sensor-wiring.md.
//
// Index = raw octant (0 = 0deg boat-relative, then 45deg per step).
//
// These replace an invented placeholder that was not merely uncalibrated but
// wrong enough to break decoding outright - octant 2 (90deg) was off by
// ~2000 counts, and since readRawOctant() takes the *nearest* entry, bad
// anchors map real positions onto unrelated octants rather than degrading
// into slightly-wrong readings (buglog bug-034). If this table is ever
// regenerated, keep that property in mind: it is a correctness input, not a
// tuning knob.
//
// Re-measure if the pull-up value, the cable run, or the sensor unit itself
// changes - all three shift the divider. The readings are not tightly
// spaced (the closest pair of adjacent octants is ~360 counts apart), so
// normal drift is not a concern.
constexpr int kOctantAdcReadings[8] = {
    2943, 1663, 209, 572, 974, 2315, 3855, 3465,
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
