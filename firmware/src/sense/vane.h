#pragma once

#include <Arduino.h>

// Resistive-divider wind vane (salvaged WH1080/WH1090) - the divider has 8
// distinct resistance steps, one per native direction, read as an analog
// voltage and mapped to the nearest of 8 raw octants (boat-relative, not yet
// corrected for yaw - see correct/wind_direction.h).
//
// Wiring: RJ11 pins 1 & 4 (outer pair, no polarity) form a 2-wire variable
// resistor (688ohm-120kohm across the 8 octants per the manufacturer's
// datasheet) - needs an EXTERNAL 10kohm pull-up from 3.3V to this pin,
// since GPIO34 (ADC1-only) has no internal pull resistor available, unlike
// most other ESP32 GPIOs. Full pinout, resistance table, and the
// pull-up circuit diagram: docs/hardware/wind-sensor-wiring.md.
class Vane {
public:
    void begin(uint8_t analogPin);

    // Returns the raw octant (0-7) nearest to the current analog reading.
    int readRawOctant();

private:
    uint8_t pin_ = 0;
};
