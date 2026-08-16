#include "sense/anemometer.h"

volatile unsigned long Anemometer::pulseCount_ = 0;

void Anemometer::begin(uint8_t pin) {
    pin_ = pin;
    pinMode(pin_, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin_), onPulseISR, FALLING);
}

unsigned long Anemometer::readAndResetPulseCount() {
    noInterrupts();
    unsigned long count = pulseCount_;
    pulseCount_ = 0;
    interrupts();
    return count;
}

unsigned long Anemometer::pulseCountSnapshot() const {
    return pulseCount_;
}

void IRAM_ATTR Anemometer::onPulseISR() {
    pulseCount_++;
}
