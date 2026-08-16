#include "indicate/pattern.h"

bool operator==(const LanePattern &a, const LanePattern &b) {
    return a.flashes == b.flashes && a.onMs == b.onMs && a.offMs == b.offMs &&
           a.pauseMs == b.pauseMs && a.oneShot == b.oneShot && a.inverted == b.inverted;
}

bool operator!=(const LanePattern &a, const LanePattern &b) {
    return !(a == b);
}

bool isLit(const LanePattern &pattern, unsigned long nowMs, unsigned long phase0) {
    bool lit = false;
    unsigned long slotMs = static_cast<unsigned long>(pattern.onMs) + pattern.offMs;
    if (pattern.flashes > 0 && slotMs > 0) {
        // Unsigned arithmetic: correct across a millis() rollover without a
        // special case, since (now - phase0) stays the true elapsed time.
        unsigned long elapsed = nowMs - phase0;
        unsigned long burstMs = static_cast<unsigned long>(pattern.flashes) * slotMs;
        unsigned long periodMs = burstMs + pattern.pauseMs;
        unsigned long t = elapsed;
        if (!pattern.oneShot) {
            t = elapsed % periodMs;  // periodMs >= burstMs > 0 here
        }
        if (t < burstMs) {
            lit = (t % slotMs) < pattern.onMs;
        }
    }
    return pattern.inverted ? !lit : lit;
}

LanePattern lanePatternOff() {
    return LanePattern();
}

LanePattern lanePatternSolid() {
    // offMs and pauseMs both zero, so the period is one on-phase long and
    // the lane is lit at every instant. No special case in isLit().
    LanePattern p;
    p.flashes = 1;
    p.onMs = 1000;
    return p;
}

LanePattern lanePatternSlowBlink() {
    LanePattern p;
    p.flashes = 1;
    p.onMs = kSlowBlinkHalfMs;
    p.offMs = kSlowBlinkHalfMs;
    return p;
}

LanePattern lanePatternFastBlink() {
    LanePattern p;
    p.flashes = 1;
    p.onMs = kFastBlinkHalfMs;
    p.offMs = kFastBlinkHalfMs;
    return p;
}

LanePattern lanePatternPulse(uint16_t onMs, uint16_t periodMs) {
    LanePattern p;
    p.flashes = 1;
    p.onMs = onMs;
    p.pauseMs = (periodMs > onMs) ? static_cast<uint16_t>(periodMs - onMs) : 0;
    return p;
}

LanePattern lanePatternDoublePulse(uint16_t onMs, uint16_t gapMs, uint16_t periodMs) {
    LanePattern p;
    p.flashes = 2;
    p.onMs = onMs;
    p.offMs = gapMs;
    unsigned long burst = 2UL * (static_cast<unsigned long>(onMs) + gapMs);
    p.pauseMs = (periodMs > burst) ? static_cast<uint16_t>(periodMs - burst) : 0;
    return p;
}

LanePattern lanePatternCode(int flashes, uint16_t periodMs) {
    if (flashes <= 0) {
        return lanePatternOff();
    }
    LanePattern p;
    p.flashes = static_cast<uint8_t>(flashes);
    p.onMs = kCodeFlashMs;
    p.offMs = kCodeFlashMs;
    unsigned long burst = static_cast<unsigned long>(flashes) * 2UL * kCodeFlashMs;
    // A high code with a short period would otherwise run its flashes back
    // to back with no gap, which cannot be counted. Guarantee a pause of at
    // least one flash slot so "N flashes, then a gap" always reads as such.
    unsigned long minPause = 2UL * kCodeFlashMs;
    unsigned long pause = (periodMs > burst + minPause) ? (periodMs - burst) : minPause;
    p.pauseMs = static_cast<uint16_t>(pause);
    return p;
}

LanePattern lanePatternOneShot(int flashes, uint16_t onMs, uint16_t offMs) {
    if (flashes <= 0) {
        return lanePatternOff();
    }
    LanePattern p;
    p.flashes = static_cast<uint8_t>(flashes);
    p.onMs = onMs;
    p.offMs = offMs;
    p.oneShot = true;
    return p;
}

LanePattern lanePatternInverted(const LanePattern &pattern) {
    LanePattern p = pattern;
    p.inverted = !p.inverted;
    return p;
}
