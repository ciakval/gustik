#pragma once

// Hard-iron calibration offsets for the magnetometer, deliberately
// configurable data rather than hardcoded magic numbers (Story 1.2 AC3) -
// must be measured on the actual installed unit/boat before deployment,
// see TODO.md. Soft-iron correction is deferred (v1 accepts hard-iron-only
// accuracy, sufficient for 8-octant resolution per NFR-6).
struct MagnetometerCalibration {
    double hardIronOffsetX;
    double hardIronOffsetY;
};

// Converts calibrated raw magnetometer X/Y readings into a compass heading
// in degrees [0, 360), 0 = north. Pure 2D heading (no tilt compensation -
// out of scope for v1's 8-octant resolution, NFR-6).
double magnetometerHeadingDegrees(double rawX, double rawY, const MagnetometerCalibration &calibration);

// Combines the vane's raw octant reading (0-7, relative to the boat's bow)
// with the boat's current yaw/heading (degrees, from
// magnetometerHeadingDegrees) into a north-relative wind octant (0-7,
// AD-5 - never degrees). As the boat swings on its mooring, this correction
// is what keeps the reported wind direction stable relative to true wind
// (FR-2) instead of drifting with the boat's rotation.
int correctWindDirectionOctant(int rawVaneOctant, double yawDegrees);
