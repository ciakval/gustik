#include "sense/magnetometer.h"
#include <Wire.h>

namespace {
// QMC5883L register map (also compatible with common HMC5883L-labeled
// clones actually shipping the QMC5883L die - see header comment).
constexpr uint8_t kI2cAddress = 0x0D;
constexpr uint8_t kRegDataOutXLow = 0x00;
constexpr uint8_t kRegControl1 = 0x09;
// Control1: mode=continuous, ODR=50Hz, full scale=2G, OSR=512
constexpr uint8_t kControl1Continuous50Hz2G = 0b00011101;
} // namespace

void Magnetometer::begin() {
    Wire.begin();
    Wire.beginTransmission(kI2cAddress);
    Wire.write(kRegControl1);
    Wire.write(kControl1Continuous50Hz2G);
    Wire.endTransmission();
}

void Magnetometer::readRawXY(double &x, double &y) {
    Wire.beginTransmission(kI2cAddress);
    Wire.write(kRegDataOutXLow);
    Wire.endTransmission(false);
    Wire.requestFrom(kI2cAddress, static_cast<uint8_t>(4));

    int16_t rawX = Wire.read() | (Wire.read() << 8);
    int16_t rawY = Wire.read() | (Wire.read() << 8);

    x = static_cast<double>(rawX);
    y = static_cast<double>(rawY);
}
