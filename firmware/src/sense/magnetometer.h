#pragma once

// HMC5883L/QMC5883L-compatible magnetometer (I2C). Real HMC5883L chips are
// discontinued - see cerebrum.md Decision Log and TODO.md: the actual
// purchased module's chip must be confirmed before wiring this up against a
// specific vendor library (architecture spine pins a dual-chip-compatible
// one, e.g. DFRobot_QMC5883).
class Magnetometer {
public:
    void begin();

    // Raw, uncalibrated X/Y magnetometer axis readings. Calibration
    // (hard-iron offset) is applied downstream in
    // correct::magnetometerHeadingDegrees, not here - see Story 1.2 AC3.
    void readRawXY(double &x, double &y);
};
