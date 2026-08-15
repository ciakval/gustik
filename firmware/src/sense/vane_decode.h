#pragma once

// Pure ADC-to-octant decoding for the WH1080/WH1090 wind vane.
//
// Deliberately Arduino.h-free and separated from sense/vane.cpp so it can be
// unit-tested in the `native` env - this is a correctness-critical lookup
// (see vane_decode.cpp's header comment for how it was got wrong once), and
// leaving it inside the hardware-coupled class made it untestable.

// Number of physically distinct positions the vane can rest at. The vane has
// 8 reed switches but 16 detents: adjacent switches close together, putting
// their resistors in parallel. See vane_decode.cpp.
constexpr int kVaneAnchorCount = 16;

// Maps a raw 12-bit ADC reading to the nearest known vane position and
// returns that position's octant (0-7, 0 = 0deg boat-relative, 45deg per
// step). Total function: every input returns some octant.
int vaneOctantForAdc(int adcReading);
