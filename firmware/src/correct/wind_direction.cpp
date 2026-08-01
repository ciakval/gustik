#include "correct/wind_direction.h"
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;

int mod8(int value) {
    return ((value % 8) + 8) % 8;
}
} // namespace

double magnetometerHeadingDegrees(double rawX, double rawY, const MagnetometerCalibration &calibration) {
    double x = rawX - calibration.hardIronOffsetX;
    double y = rawY - calibration.hardIronOffsetY;

    double headingRad = std::atan2(y, x);
    double headingDeg = headingRad * (180.0 / kPi);
    if (headingDeg < 0.0) {
        headingDeg += 360.0;
    }
    return headingDeg;
}

int correctWindDirectionOctant(int rawVaneOctant, double yawDegrees) {
    int yawOctant = mod8(static_cast<int>(std::lround(yawDegrees / 45.0)));
    return mod8(rawVaneOctant + yawOctant);
}
