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

// The band the divider can physically produce with the vane connected. The
// measured anchors span 107..3855 counts; outside these bounds the pin is
// either shorted to GND (0) or open, in which case the external 10kohm
// pull-up takes it to full scale (~4095). Both are wiring faults, and both
// otherwise decode to a confident, stable octant with no error anywhere -
// which is exactly how bug-045 and bug-050 hid.
constexpr int kVaneAdcMinPlausible = 60;
constexpr int kVaneAdcMaxPlausible = 4000;

// Is the vane plausibly connected? Deliberately a RANGE check and not a
// "within N counts of the nearest anchor" check: while the vane turns it
// passes through genuinely intermediate resistances, so an anchor-proximity
// test would false-alarm on a working sensor in a gusty wind. The failure
// modes worth signalling on the boat (open circuit, short) both leave the
// band entirely. Feeds the status panel's sensor mode (Q13) and fault
// code 8; nothing in the measurement path consults it.
bool vaneAdcPlausible(int adcReading);
