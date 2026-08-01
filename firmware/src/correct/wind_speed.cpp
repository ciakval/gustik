#include "correct/wind_speed.h"

double pulsesToWindSpeedMs(unsigned long pulseCount, double intervalSeconds, const AnemometerCalibration &calibration) {
    if (intervalSeconds <= 0.0) {
        return 0.0;
    }
    double hz = static_cast<double>(pulseCount) / intervalSeconds;
    return hz * calibration.metersPerSecondPerHz;
}
