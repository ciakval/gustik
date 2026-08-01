#include <Arduino.h>
#include "sense/anemometer.h"
#include "correct/wind_speed.h"

namespace {
// Sampling interval per FR-1 (~2-5s) - exact value pending flash/buffer
// capacity check, see TODO.md and epics.md Story 1.1/2.1 open note.
constexpr unsigned long kSampleIntervalMs = 3000;
constexpr uint8_t kAnemometerPin = 27;

// TODO(calibration): measure against a reference anemometer before
// deployment - see correct/wind_speed.h.
constexpr AnemometerCalibration kAnemometerCalibration{.metersPerSecondPerHz = 1.2};

Anemometer anemometer;
unsigned long lastSampleAt = 0;

// Latest computed reading, in m/s only (FR-1 scope - no other unit computed
// at this stage, no yaw/direction correction until Story 1.2).
double windSpeedMs = 0.0;
} // namespace

void setup() {
    anemometer.begin(kAnemometerPin);
    lastSampleAt = millis();
}

void loop() {
    unsigned long now = millis();
    if (now - lastSampleAt < kSampleIntervalMs) {
        return;
    }
    double intervalSeconds = (now - lastSampleAt) / 1000.0;
    lastSampleAt = now;

    unsigned long pulses = anemometer.readAndResetPulseCount();
    windSpeedMs = pulsesToWindSpeedMs(pulses, intervalSeconds, kAnemometerCalibration);
}
