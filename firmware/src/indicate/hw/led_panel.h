#pragma once

#include <Arduino.h>

#include "indicate/panel.h"

// The nine-digitalWrite half of the status panel. Hardware-coupled
// (Arduino.h) and therefore not in the native test build - which is the
// point: everything that decides WHAT to light lives in indicate/panel.cpp
// and is tested there, and this file only resolves patterns to pin levels.
//
// Wiring is active high, one 330 ohm resistor per lane:
//     GPIO -> [330R] -> LED anode, cathode -> GND
// so digitalWrite(pin, HIGH) lights it. All four kit colours (Vf 2.0-2.4 V,
// blue 2.6 V measured) draw 2.1-3.9 mA at that value, far inside the
// ESP32's 12 mA recommended per-pin figure.
//
// Fitting fewer than nine LEDs is supported and costs no code change:
// driving a GPIO with nothing attached is harmless. Priority order if some
// are left off: status RED, status BLUE, status GREEN, status YELLOW, then
// the detail group.
//
// Full pin map, the GPIO6-11 flash-bus warning and the reasoning for every
// choice: docs/superpowers/specs/2026-08-16-status-led-panel-design.md
// section 4.1.
struct LedPanelPins {
    uint8_t status[kStatusLaneCount];
    uint8_t detail[kDetailPositionCount];
};

class LedPanel {
public:
    void begin(const LedPanelPins &pins);

    // Resolves every lane's pattern at `nowMs` and writes the nine pins.
    // No allocation, no delay, no Serial - call it as often as loop() runs.
    void render(const PanelOutputs &outputs, unsigned long nowMs);

    // Drives all nine low. Used when leds.enabled=false, so the pins are
    // left low from boot rather than floating at whatever the ROM left.
    void allOff();

private:
    LedPanelPins pins_{};
    bool begun_ = false;
};
