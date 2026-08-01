#include <Arduino.h>
#include "sense/anemometer.h"
#include "sense/vane.h"
#include "sense/magnetometer.h"
#include "correct/wind_speed.h"
#include "correct/wind_direction.h"

namespace {
// Sampling interval per FR-1 (~2-5s) - exact value pending flash/buffer
// capacity check, see TODO.md and epics.md Story 1.1/2.1 open note.
constexpr unsigned long kSampleIntervalMs = 3000;
constexpr uint8_t kAnemometerPin = 27;
constexpr uint8_t kVanePin = 34;

// TODO(calibration): measure against a reference anemometer before
// deployment - see correct/wind_speed.h.
constexpr AnemometerCalibration kAnemometerCalibration{.metersPerSecondPerHz = 1.2};

// TODO(calibration): hard-iron calibration for the actual installed
// magnetometer/boat mount - see correct/wind_direction.h and TODO.md.
constexpr MagnetometerCalibration kMagnetometerCalibration{.hardIronOffsetX = 0.0, .hardIronOffsetY = 0.0};

Anemometer anemometer;
Vane vane;
Magnetometer magnetometer;
unsigned long lastSampleAt = 0;

// Latest computed readings. Wind speed in m/s (FR-1); wind direction as a
// north-relative octant 0-7 (FR-2, AD-5 - never degrees).
double windSpeedMs = 0.0;
int windDirOctant = 0;
} // namespace

void setup() {
    anemometer.begin(kAnemometerPin);
    vane.begin(kVanePin);
    magnetometer.begin();
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

    int rawVaneOctant = vane.readRawOctant();
    double magX, magY;
    magnetometer.readRawXY(magX, magY);
    double yawDegrees = magnetometerHeadingDegrees(magX, magY, kMagnetometerCalibration);
    windDirOctant = correctWindDirectionOctant(rawVaneOctant, yawDegrees);
}
