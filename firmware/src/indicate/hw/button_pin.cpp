#include "indicate/hw/button_pin.h"

void ButtonPin::begin(uint8_t pin) {
    pin_ = pin;
    begun_ = true;
    pinMode(pin_, INPUT_PULLUP);
}

bool ButtonPin::isPressed() const {
    if (!begun_) {
        return false;
    }
    return digitalRead(pin_) == LOW;
}
