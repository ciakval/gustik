#pragma once

#include <Arduino.h>

// Resistive-divider wind vane (salvaged WH1080/WH1090) - the divider has 8
// distinct resistance steps, one per native direction, read as an analog
// voltage and mapped to the nearest of 8 raw octants (boat-relative, not yet
// corrected for yaw - see correct/wind_direction.h).
class Vane {
public:
    void begin(uint8_t analogPin);

    // Returns the raw octant (0-7) nearest to the current analog reading.
    int readRawOctant();

private:
    uint8_t pin_ = 0;
};
