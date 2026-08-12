#include "sense/magnetometer.h"
#include <Wire.h>

namespace {
// QMC5883P register map - see header comment for chip confirmation.
// Mirrors scripts/src/gustik_scripts/qmc5883p.py, the bench driver this
// was reverse-engineered/confirmed against.
constexpr uint8_t kI2cAddress = 0x2C;
constexpr uint8_t kRegDataStart = 0x01; // X_L,X_H,Y_L,Y_H,Z_L,Z_H
constexpr uint8_t kRegCtrl1 = 0x0A;     // mode + output data rate
constexpr uint8_t kRegCtrl2 = 0x0B;     // field range
constexpr uint8_t kRegMystery0D = 0x0D; // undocumented, part of QST's reference init sequence
constexpr uint8_t kRegSign0x29 = 0x29;  // undocumented, part of QST's reference init sequence

// 8G range (CTRL2 RNG bits 0b10, shifted into bits 3:2) - must match the
// range scripts/qmc5883p-calibration.json's hard-iron offsets were captured
// at (offsets are raw LSB, not portable across ranges - see main.cpp).
constexpr uint8_t kCtrl2Range8G = 0b00001000;
// Continuous mode, 100Hz ODR (0xC0 upper nibble is QST's reference value;
// bits 3:2 = ODR, bits 1:0 = mode=continuous(0b11)).
constexpr uint8_t kCtrl1Continuous100Hz = 0xCB;

void writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(kI2cAddress);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}
} // namespace

void Magnetometer::begin() {
    Wire.begin();
    writeRegister(kRegMystery0D, 0x40);
    writeRegister(kRegSign0x29, 0x06);
    writeRegister(kRegCtrl2, kCtrl2Range8G);
    writeRegister(kRegCtrl1, kCtrl1Continuous100Hz);
}

void Magnetometer::readRawXY(double &x, double &y) {
    Wire.beginTransmission(kI2cAddress);
    Wire.write(kRegDataStart);
    Wire.endTransmission(false);
    // X_L,X_H,Y_L,Y_H only - Z is skipped (v1 has no tilt compensation,
    // see wind_direction.h).
    Wire.requestFrom(kI2cAddress, static_cast<uint8_t>(4));

    int16_t rawX = Wire.read() | (Wire.read() << 8);
    int16_t rawY = Wire.read() | (Wire.read() << 8);

    // Confirmed real mount is up=-z, forward=+x (chip mounted upside down -
    // verified with --check-rotation, see scripts/README.md "Verify the
    // mount"). In that frame left = up x forward = -y, and heading is the
    // bearing of (forward, left) - so Y is negated here rather than baking
    // a mount-specific sign flip into correct/wind_direction.cpp's generic
    // atan2(y, x). main.cpp's MagnetometerCalibration.hardIronOffsetY is
    // correspondingly negated relative to qmc5883p-calibration.json's raw
    // offset - see the comment there.
    x = static_cast<double>(rawX);
    y = static_cast<double>(-rawY);
}
