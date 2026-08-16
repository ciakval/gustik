#pragma once

#include <Arduino.h>

// Interrupt-driven reed-switch pulse counter. Hardware-coupled (Arduino.h) -
// not part of the native test build; see correct/wind_speed.h for the
// testable pulses -> m/s conversion this feeds.
//
// Wiring: salvaged WH1080/WH1090 anemometer, RJ11 pins 2 & 3 (inner pair,
// no polarity) - one wire to this pin, the other to GND. Relies on the
// ESP32's internal pull-up (begin() sets INPUT_PULLUP), no external
// resistor needed. Full RJ11 pinout + wind vane wiring (which DOES need
// an external resistor - GPIO34 has no internal pull option) in
// docs/hardware/wind-sensor-wiring.md.
class Anemometer {
public:
    void begin(uint8_t pin);

    // Returns the pulse count accumulated since the last call and resets
    // the counter, so callers get one sampling-interval's worth each time.
    unsigned long readAndResetPulseCount();

    // A non-resetting read of the same counter, for the status panel's live
    // reed-closure indicator (sensor mode, detail position 1). Deliberately
    // separate and side-effect free: the measurement path above is the only
    // thing allowed to reset the counter, which is what keeps the panel
    // strictly downstream of the sensors (design constraint C3).
    //
    // No noInterrupts() needed - a 32-bit aligned read is atomic on this
    // core, and the panel only compares successive snapshots for an
    // increase. A decrease (every time readAndResetPulseCount() runs) is
    // treated as a resync, not an event.
    unsigned long pulseCountSnapshot() const;

private:
    uint8_t pin_ = 0;
    static void IRAM_ATTR onPulseISR();
    static volatile unsigned long pulseCount_;
};
