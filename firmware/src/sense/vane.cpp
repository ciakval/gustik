#include "sense/vane.h"

#include "sense/vane_decode.h"

void Vane::begin(uint8_t analogPin) {
    pin_ = analogPin;
    pinMode(pin_, INPUT);
}

int Vane::readRawOctant() {
    // Decoding lives in sense/vane_decode.cpp: it is pure, correctness-
    // critical, and unit-tested in the `native` env (test/test_vane_decode).
    return vaneOctantForAdc(analogRead(pin_));
}
