// Status LED panel wiring check.  [env:panel_diag], real hardware only.
//
// Dumb firmware, eyes as the analysis - the project's standing bring-up
// preference (cerebrum.md). This sketch decides nothing: it walks the nine
// panel LEDs one at a time in physical row order and dumps raw button
// transitions. Whether the wiring is right is a judgement made by looking at
// the board, not by the ESP32.
//
// It proves the WIRING. The panel's logic is pure and already under test in
// [env:native] (test/test_panel_*, test/test_button).
//
// What to look for:
//   * Exactly one LED lit at a time, moving strictly left to right along the
//     physical row, then repeating. Any LED that lights out of order is two
//     swapped jumpers - nine LEDs means nine chances to make that mistake.
//   * Every LED lights at all, at roughly the same brightness. One visibly
//     dimmer lane at the same 330 ohm resistor means a higher-Vf part from a
//     different batch (check with the LED in series with 1 kohm across 3.3 V,
//     measuring across the LED itself).
//   * Colours in the row order R Y G B | G G Y Y R.
//   * `BTN <micros> <level>` lines when the button is pressed and released -
//     level 0 is pressed (active LOW). Bounce shows up as a burst of lines
//     within a few milliseconds of each other, which is exactly what the
//     30 ms debounce in indicate/button.cpp exists to swallow.
//
// Deliberately depends on nothing else in src/ - no WiFi, no config.txt, no
// sensors - so anything it reports is unambiguously about the panel.
//
//     pio run -e panel_diag -t upload && pio device monitor -b 115200
//
// Put the station firmware back afterwards:
//     pio run -e esp32dev -t upload

#include <Arduino.h>

namespace {

// Physical row order, left to right: R Y G B | G G Y Y R.
// Kept as literals rather than including indicate/hw/panel_pins.h, so this
// sketch stays a standalone description of what is expected on the bench.
// As built: the status group runs DOWN the left header column, the detail
// group runs UP the right one. The list is in PHYSICAL ROW order, which is
// the only order that matters here - see indicate/hw/panel_pins.h.
constexpr uint8_t kRowPins[] = {32, 33, 25, 26, 4, 16, 17, 18, 19};
constexpr const char *kRowNames[] = {
    "status RED", "status YELLOW", "status GREEN", "status BLUE",
    "detail 1 GREEN", "detail 2 GREEN", "detail 3 YELLOW", "detail 4 YELLOW", "detail 5 RED",
};
constexpr size_t kRowCount = sizeof(kRowPins) / sizeof(kRowPins[0]);
constexpr uint8_t kButtonPin = 13;
constexpr unsigned long kStepMs = 2000;

size_t step = 0;
unsigned long lastStepAt = 0;
int lastButtonLevel = -1;

} // namespace

void setup() {
    Serial.begin(115200);
    for (size_t i = 0; i < kRowCount; i++) {
        pinMode(kRowPins[i], OUTPUT);
        digitalWrite(kRowPins[i], LOW);
    }
    pinMode(kButtonPin, INPUT_PULLUP);
    Serial.println();
    Serial.println("--- gustik panel_diag ---");
    Serial.printf("walking %u LEDs, %lu ms each, left to right\n", (unsigned)kRowCount, kStepMs);
    Serial.printf("button on GPIO%u, INPUT_PULLUP (0 = pressed)\n", kButtonPin);
    lastStepAt = millis();
}

void loop() {
    unsigned long now = millis();

    if (now - lastStepAt >= kStepMs) {
        lastStepAt = now;
        digitalWrite(kRowPins[step], LOW);
        step = (step + 1) % kRowCount;
        digitalWrite(kRowPins[step], HIGH);
        Serial.printf("LED %u GPIO%-2u  %s\n", (unsigned)step, kRowPins[step], kRowNames[step]);
    }

    int level = digitalRead(kButtonPin);
    if (level != lastButtonLevel) {
        lastButtonLevel = level;
        // Raw, unfiltered, one line per transition - bounce included, on
        // purpose. Interpreting it is the host's job, or the reader's.
        Serial.printf("BTN %lu %d\n", micros(), level);
    }
}
