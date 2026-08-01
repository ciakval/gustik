#pragma once

// Hardware-specific calibration constant, deliberately not a hardcoded magic
// number in the conversion formula below (see Story 1.2 AC3 for the same
// principle applied to magnetometer calibration). The real value for the
// salvaged WH1080/WH1090 anemometer must be measured against a reference
// before deployment - see TODO.md.
struct AnemometerCalibration {
    double metersPerSecondPerHz;
};

// Converts an interrupt-counted pulse count over a sampling interval into a
// wind speed in m/s. Never returns NaN/Inf: a zero or negative interval
// (should not happen with a fixed sampling loop, but is not this function's
// job to assume) yields 0.0 rather than propagating a divide-by-zero.
double pulsesToWindSpeedMs(unsigned long pulseCount, double intervalSeconds, const AnemometerCalibration &calibration);
