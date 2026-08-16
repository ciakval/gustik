// Anemometer bring-up / pulse capture.
//
// Prints RAW GPIO edge events and nothing else. All interpretation happens on
// the host or in your head:
//
//   ~/.platformio/penv/bin/pio run -e pulse_diag -t upload
//   ~/.platformio/penv/bin/pio device monitor -b 115200
//
// Deliberately self-contained - it does not include sense/anemometer.h, read
// config.txt, or touch WiFi/I2C/the vane, so anything it reports is
// unambiguously about the anemometer reed switch and its wiring.
//
// Two differences from the station firmware, both on purpose:
//   * trigger is CHANGE, not FALLING - "some edges but no falling edges" and
//     "no edges at all" are different faults, and the station firmware cannot
//     tell them apart.
//   * every edge is timestamped in micros() and printed individually, so
//     contact bounce (a burst of edges microseconds apart) is visible as
//     itself rather than as an inflated pulse count.
//
// If EDGE lines never appear at all, short GPIO27 to GND with a jumper wire
// (sensor unplugged or not - it is a plain reed switch either way): that is
// electrically the same thing the anemometer does once per rotation. Edges on
// the jumper but not on the sensor means the fault is past the ESP32 pin -
// wrong RJ11 pair, bad crimp, or a dead switch. No edges even on the jumper
// means the fault is the pin, the board, or this sketch.
//
// History: written 2026-08-16 for a `pulses=0` report (bug-059). It was never
// needed - the fault was a faulty wire between the RJ11 cable and GPIO27, found
// before this was flashed. Kept anyway, on request: nothing else in the tree can
// tell "the pin never moves" apart from "the pin moves and the firmware misses
// it", and a passive sensor on a boat will produce this symptom again.

#include <Arduino.h>

namespace {
constexpr uint8_t kPulsePin = 27; // must match main.cpp's kAnemometerPin
constexpr unsigned long kSerialBaudRate = 115200;
constexpr unsigned long kSummaryIntervalMs = 1000;
constexpr uint32_t kRingSize = 256; // power of two - index is `& (kRingSize-1)`

// Fields are individually volatile rather than the array being `volatile
// Edge[]`: a volatile struct has no usable copy assignment, so whole-struct
// reads/writes would not compile.
struct Edge {
    volatile unsigned long atMicros;
    volatile uint8_t level;
};

Edge ring[kRingSize];
volatile uint32_t head = 0; // written by the ISR only
uint32_t tail = 0;          // read/written by loop() only

volatile unsigned long fallingTotal = 0;
volatile unsigned long risingTotal = 0;

// No Serial in the ISR - it takes locks and would deadlock or drop edges.
// The ISR only stamps micros() into the ring; loop() does all the printing.
void IRAM_ATTR onEdgeISR() {
    unsigned long at = micros();
    uint8_t level = digitalRead(kPulsePin) ? 1 : 0;
    if (level) {
        risingTotal++;
    } else {
        fallingTotal++;
    }
    Edge &slot = ring[head & (kRingSize - 1)];
    slot.atMicros = at;
    slot.level = level;
    head++;
}
} // namespace

void setup() {
    Serial.begin(kSerialBaudRate);
    pinMode(kPulsePin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(kPulsePin), onEdgeISR, CHANGE);
    delay(50); // let the pull-up settle before reporting the idle level
    Serial.println();
    Serial.printf("# gustik pulse_diag pin=%u mode=INPUT_PULLUP trigger=CHANGE\n", kPulsePin);
    Serial.printf("# idle level=%d (expect 1 - pull-up holds the pin high with the reed switch open)\n",
                  digitalRead(kPulsePin));
    Serial.println("# EDGE <micros> <level>   one line per GPIO transition, level after the edge");
    Serial.println("# TICK <millis> level=<0|1> falling=<total> rising=<total> f1s=<falling last second> dropped=<n>");
    Serial.println("# no EDGE lines? short pin 27 to GND with a wire - see the comment at the top of this file");
}

void loop() {
    static unsigned long lastSummaryAt = 0;
    static unsigned long fallingAtLastSummary = 0;

    // Drain whatever the ISR queued. Reading `head` once per pass keeps the
    // window well-defined even if edges keep arriving while we print.
    uint32_t currentHead = head;
    unsigned long dropped = 0;
    if (currentHead - tail > kRingSize) {
        dropped = currentHead - tail - kRingSize;
        tail = currentHead - kRingSize; // oldest entries were overwritten
    }
    while (tail != currentHead) {
        const Edge &edge = ring[tail & (kRingSize - 1)];
        Serial.printf("EDGE %lu %u\n", edge.atMicros, static_cast<unsigned>(edge.level));
        tail++;
    }
    if (dropped > 0) {
        Serial.printf("# dropped %lu edge(s) - ring overflow, bounce faster than the serial link\n", dropped);
    }

    unsigned long now = millis();
    if (now - lastSummaryAt >= kSummaryIntervalMs) {
        lastSummaryAt = now;
        unsigned long falling = fallingTotal;
        Serial.printf("TICK %lu level=%d falling=%lu rising=%lu f1s=%lu dropped=%lu\n", now,
                      digitalRead(kPulsePin), falling, risingTotal,
                      falling - fallingAtLastSummary, dropped);
        fallingAtLastSummary = falling;
    }
}
