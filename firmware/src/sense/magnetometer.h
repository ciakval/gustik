#pragma once

// QMC5883P magnetometer (I2C, address 0x2C) - the chip actually on the
// "GY-271"/HMC5883L-silkscreened boards used for this project, confirmed
// 2026-08-11 via bench testing with a USB-I2C bridge (see
// scripts/README.md and scripts/src/gustik_scripts/qmc5883p.py, the
// reference this register map is taken from). NOT register- or
// address-compatible with the real HMC5883L (0x1E) or QMC5883L (0x0D) this
// file previously (wrongly) targeted - see buglog bug-029.
//
// I2C wiring: SDA -> GPIO21, SCL -> GPIO22 (ESP32 Arduino core's default
// Wire pins, used here since Wire.begin() is called with no explicit pin
// args), VCC -> 3.3V, GND -> GND.
class Magnetometer {
public:
    // Returns false if any init register write was NACK'd/timed out (bad
    // wiring, wrong address, unpowered chip) - the caller should surface
    // this rather than assume the chip is ready.
    bool begin();

    // Raw, uncalibrated X/left magnetometer axis readings (Y is returned
    // negated - see magnetometer.cpp for why). Calibration (hard-iron
    // offset) is applied downstream in correct::magnetometerHeadingDegrees,
    // not here - see Story 1.2 AC3.
    //
    // Returns false (x/y left unmodified) on any I2C failure - a NACK, bus
    // error, or short read. Never blocks indefinitely: begin() bounds every
    // Wire call with a timeout, so a stuck/faulty magnetometer degrades to
    // "this cycle's read failed" instead of freezing the whole sampling
    // loop (see bug-030 in buglog.json).
    bool readRawXY(double &x, double &y);
};
