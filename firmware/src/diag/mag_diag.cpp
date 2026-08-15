// Magnetometer bring-up / calibration capture.
//
// Prints RAW QMC5883P counts and nothing else. All interpretation happens on
// the host (see scripts/README.md):
//
//   ~/.platformio/penv/bin/pio run -e mag_diag -t upload
//   cd scripts && python3 -m gustik_scripts.mag_calibrate \
//       --port /dev/ttyUSB0 --tumble --seconds 60
//
// Deliberately self-contained - it does not include sense/magnetometer.h,
// read config.txt, or touch WiFi, so anything it reports is unambiguously
// about the magnetometer and its wiring. It also prints all THREE axes
// un-negated, whereas sense/magnetometer.cpp returns X/Y only with Y already
// sign-flipped for the mount: calibration has to see the chip's own frame,
// and the vertical axis is what makes a tumble calibration possible.
//
// Scaffolding, not a deliverable - delete this file and its platformio.ini
// env once the constants are in config.txt (see cerebrum.md).

#include <Arduino.h>
#include <Wire.h>

namespace {
constexpr uint8_t kI2cAddress = 0x2C;
constexpr uint8_t kRegDataStart = 0x01; // X_L,X_H,Y_L,Y_H,Z_L,Z_H
constexpr uint8_t kRegCtrl1 = 0x0A;
constexpr uint8_t kRegCtrl2 = 0x0B;
constexpr uint8_t kRegMystery0D = 0x0D; // QST reference init sequence
constexpr uint8_t kRegSign0x29 = 0x29;  // QST reference init sequence

// 8G range. Hard-iron offsets come out in raw LSB and do NOT carry across
// ranges, so this must match sense/magnetometer.cpp's kCtrl2Range8G or the
// captured calibration is meaningless in the station firmware.
constexpr uint8_t kCtrl2Range8G = 0b00001000;
constexpr uint8_t kCtrl1Continuous100Hz = 0xCB;

void writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(kI2cAddress);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}
} // namespace

void setup() {
    Serial.begin(115200);
    Wire.begin();
    Wire.setTimeOut(1000); // never let a stuck bus freeze the loop (bug-030)
    writeRegister(kRegMystery0D, 0x40);
    writeRegister(kRegSign0x29, 0x06);
    writeRegister(kRegCtrl2, kCtrl2Range8G);
    writeRegister(kRegCtrl1, kCtrl1Continuous100Hz);
    Serial.println();
    Serial.println("# gustik mag_diag chip=QMC5883P addr=0x2C range=8G fields=x,y,z");
}

void loop() {
    Wire.beginTransmission(kI2cAddress);
    Wire.write(kRegDataStart);
    if (Wire.endTransmission(false) != 0) {
        Serial.println("# I2C register select failed (NACK/bus error)");
        delay(200);
        return;
    }
    if (Wire.requestFrom(kI2cAddress, static_cast<uint8_t>(6)) != 6) {
        Serial.println("# I2C short read");
        delay(200);
        return;
    }

    // Read into a buffer first: `Wire.read() | (Wire.read() << 8)` leaves the
    // two calls unsequenced, and a silently byte-swapped sample would corrupt
    // the calibration this sketch exists to produce.
    uint8_t bytes[6];
    for (uint8_t i = 0; i < 6; i++) {
        bytes[i] = Wire.read();
    }
    int16_t x = static_cast<int16_t>(bytes[0] | (bytes[1] << 8));
    int16_t y = static_cast<int16_t>(bytes[2] | (bytes[3] << 8));
    int16_t z = static_cast<int16_t>(bytes[4] | (bytes[5] << 8));

    Serial.printf("MAG %d %d %d\n", x, y, z);
    delay(20); // 50 Hz - well inside 115200 baud and the chip's 100 Hz ODR
}
