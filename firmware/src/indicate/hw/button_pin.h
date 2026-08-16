#pragma once

#include <Arduino.h>

// The panel's one button: a momentary switch from the pin to GND, read with
// the ESP32's internal pull-up, so it is active LOW.
//
// With no button fitted the pin reads HIGH forever and isPressed() is
// always false - the "absent hardware is a no-op" invariant (design section
// 7.3) needs no code of its own here. A bare jumper poked into the pin's
// breadboard row and tapped against the ground rail is electrically
// identical to a switch and is enough to exercise every mode on the bench.
//
// GPIO13 is deliberate: it is not a strapping pin and has no boot-time
// output glitch (GPIO14 does, which is why the button is not there), and it
// sits directly below a GND pin on the left header column. Route its wire
// AWAY from the bottom of the header - SD2/SD3/CMD and the battery pack's
// V5 pin are down there, and CMD is GPIO11 on the SPI flash bus.
class ButtonPin {
public:
    void begin(uint8_t pin);

    // True while the button is held down. Debouncing and gesture decoding
    // are indicate/button.h's job (pure, tested); this is a bare read.
    bool isPressed() const;

private:
    uint8_t pin_ = 0;
    bool begun_ = false;
};
