#include "indicate/hw/led_panel.h"

void LedPanel::begin(const LedPanelPins &pins) {
    pins_ = pins;
    begun_ = true;
    for (int i = 0; i < kStatusLaneCount; i++) {
        pinMode(pins_.status[i], OUTPUT);
        digitalWrite(pins_.status[i], LOW);
    }
    for (int i = 0; i < kDetailPositionCount; i++) {
        pinMode(pins_.detail[i], OUTPUT);
        digitalWrite(pins_.detail[i], LOW);
    }
}

void LedPanel::render(const PanelOutputs &outputs, unsigned long nowMs) {
    if (!begun_) {
        return;
    }
    for (int i = 0; i < kStatusLaneCount; i++) {
        digitalWrite(pins_.status[i], isLit(outputs.status[i], nowMs) ? HIGH : LOW);
    }
    for (int i = 0; i < kDetailPositionCount; i++) {
        digitalWrite(pins_.detail[i], isLit(outputs.detail[i], nowMs) ? HIGH : LOW);
    }
}

void LedPanel::allOff() {
    if (!begun_) {
        return;
    }
    for (int i = 0; i < kStatusLaneCount; i++) {
        digitalWrite(pins_.status[i], LOW);
    }
    for (int i = 0; i < kDetailPositionCount; i++) {
        digitalWrite(pins_.detail[i], LOW);
    }
}
