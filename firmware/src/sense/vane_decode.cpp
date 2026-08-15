#include "sense/vane_decode.h"

#include <cstdlib>

namespace {

struct VaneAnchor {
    int adc;    // raw 12-bit analogRead() value at this detent
    int octant; // octant reported for it (0-7)
};

// Every position the vane can physically rest at, with the octant each one
// decodes to.
//
// WHY 16 AND NOT 8. The vane has 8 reed switches, each with its own resistor,
// but 16 detents: at the half-way positions two adjacent switches close
// together and their resistors end up in PARALLEL. Parallel resistance is
// always LOWER than either leg, so a half-detent does NOT read between the
// two octants it sits between - it lands somewhere unrelated on the ladder.
// With only the 8 primary anchors present, nearest-match sent three of those
// half-detents to a wildly wrong octant (buglog bug-038):
//     157.5deg -> 90deg (67.5deg error)
//     292.5deg ->  0deg (67.5deg error)
//     337.5deg -> 225deg (112.5deg error)
// Those are mechanically stable rest positions, so a steady wind parking the
// vane on one produced a confident, stable, badly wrong direction with no
// error signal anywhere. Listing all 16 positions bounds the worst case at
// 22.5deg, which is the floor for an 8-octant output (AD-5).
//
// TIE-BREAK: a half-detent is exactly between two octants, so the choice is a
// convention, not a measurement - each one is assigned to the HIGHER octant
// (round half up). Octant k therefore covers the arc (45k - 22.5, 45k].
//
// PROVENANCE. Measured 2026-08-15 on the real salvaged vane with the real
// 10kohm pull-up and 3.3V rail, from an 18337-sample capture of two full hand
// rotations (raw serial dump, analyzed on the host - see cerebrum.md's
// Decision Log for why bring-up is done that way). Primaries carry 1861-2932
// settled samples each; half-detents are passed through faster and carry
// 22-81, except 67.5deg at 29. Every value here is measured, not computed.
// The 8 primaries reproduce an independent earlier capture to within 1 count.
// Full method, resistances and the ADC-nonlinearity check that identified
// each half-detent: docs/hardware/wind-sensor-wiring.md.
//
// Ordered by ADC so the table reads as the ladder the hardware actually
// produces; the octant column is deliberately non-monotonic as a result.
//
// RE-MEASURE if the pull-up value, the cable run, or the sensor unit itself
// changes - all three shift the divider. This is a correctness input, not a
// tuning knob: readings map to the NEAREST entry, so a bad anchor does not
// degrade into "slightly wrong degrees", it maps real positions onto
// unrelated octants (that is how bug-034 and bug-038 both happened).
constexpr VaneAnchor kVaneAnchors[kVaneAnchorCount] = {
    // ADC   octant   position    note
    {  107,  3 },  // 112.5deg  half-detent
    {  175,  2 },  //  67.5deg  half-detent (weakest cluster, n=29)
    {  209,  2 },  //  90deg    primary
    {  339,  4 },  // 157.5deg  half-detent - was decoding as 90deg
    {  572,  3 },  // 135deg    primary
    {  803,  5 },  // 202.5deg  half-detent
    {  974,  4 },  // 180deg    primary
    { 1442,  1 },  //  22.5deg  half-detent
    { 1663,  1 },  //  45deg    primary
    { 2195,  6 },  // 247.5deg  half-detent
    { 2315,  5 },  // 225deg    primary
    { 2604,  0 },  // 337.5deg  half-detent - was decoding as 225deg
    { 2943,  0 },  //   0deg    primary
    { 3134,  7 },  // 292.5deg  half-detent - was decoding as 0deg
    { 3465,  7 },  // 315deg    primary
    { 3855,  6 },  // 270deg    primary
};

} // namespace

int vaneOctantForAdc(int adcReading) {
    int nearest = 0;
    int smallestDiff = std::abs(adcReading - kVaneAnchors[0].adc);
    for (int i = 1; i < kVaneAnchorCount; i++) {
        int diff = std::abs(adcReading - kVaneAnchors[i].adc);
        if (diff < smallestDiff) {
            smallestDiff = diff;
            nearest = i;
        }
    }
    return kVaneAnchors[nearest].octant;
}
