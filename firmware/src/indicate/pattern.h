#pragma once

#include <cstdint>

// Blink patterns for one LED lane of the status panel.
//
// Pure (no Arduino.h) so the whole vocabulary in section 5 of
// docs/superpowers/specs/2026-08-16-status-led-panel-design.md is unit
// tested in [env:native]: solid, slow/fast blink, discrete pulses, the
// N-flash fault codes, the mode banner and the all-lanes alarm.
//
// One struct covers all of them, expressed as a burst of `flashes`
// on-phases followed by a pause, repeating:
//
//     |<-on->|<off>|<-on->|<off>|<------ pause ------>|<-on->| ...
//     \___________ flashes = 2 __________/
//
// Keeping it data rather than an enum of named patterns is what makes the
// renderer trivial (indicate/hw/led_panel.cpp does nothing but call isLit()
// nine times) and what lets every mode's own table be asserted in a test.
struct LanePattern {
    uint8_t flashes = 0;    // on-phases per period; 0 = permanently dark
    uint16_t onMs = 0;      // length of each on-phase
    uint16_t offMs = 0;     // gap between on-phases inside one burst
    uint16_t pauseMs = 0;   // gap after the burst, before it repeats
    bool oneShot = false;   // run the burst once, then stay dark forever
    bool inverted = false;  // swap lit and dark (see lanePatternInverted)
};

bool operator==(const LanePattern &a, const LanePattern &b);
bool operator!=(const LanePattern &a, const LanePattern &b);

// A lane is a pattern plus the instant its cycle is anchored to, so a fault
// code always starts at flash 1 when the fault appears and a one-shot pulse
// is anchored to the event that caused it. Kept together because a pattern
// without its phase reference cannot be rendered.
struct Lane {
    LanePattern pattern;
    unsigned long phase0 = 0;
};

// Is this pattern lit at `nowMs`, given a cycle anchored at `phase0`?
//
// Unsigned subtraction throughout, so a millis() rollover (~49 days) costs
// at most one mistimed blink rather than a stuck lane.
bool isLit(const LanePattern &pattern, unsigned long nowMs, unsigned long phase0);

inline bool isLit(const Lane &lane, unsigned long nowMs) {
    return isLit(lane.pattern, nowMs, lane.phase0);
}

// --- The vocabulary of section 5.  Reading rules, learn once: -------------
//   solid       good / present / confirmed
//   slow blink  attention - working, but degraded
//   fast blink  transient - busy, connecting, or an alarm
//   pulse       a discrete event happened
//   code N      a numbered fault (section 5.3)
//   off         absent / not applicable

LanePattern lanePatternOff();
LanePattern lanePatternSolid();
LanePattern lanePatternSlowBlink();  // 1 Hz
LanePattern lanePatternFastBlink();  // 4 Hz

// A short flash of `onMs` repeating every `periodMs` - the heartbeat, and
// the anemometer's live reed pulses when used one-shot.
LanePattern lanePatternPulse(uint16_t onMs, uint16_t periodMs);

// Two short flashes per period. Used for "alive, and there is a backlog
// buffered to flash" so one lane answers both questions.
LanePattern lanePatternDoublePulse(uint16_t onMs, uint16_t gapMs, uint16_t periodMs);

// N fast flashes then a pause, repeating - the fault codes. `periodMs` is
// the whole cycle including the flashes, so the awake (2 s) and asleep
// (10 s) repeats differ only in this argument.
LanePattern lanePatternCode(int flashes, uint16_t periodMs);

// N flashes, once, then dark: the mode banner (section 5.8), the direction
// code (section 5.6) and one-shot event pulses.
LanePattern lanePatternOneShot(int flashes, uint16_t onMs, uint16_t offMs);

// Lit except for a brief dark notch - "the dot is pulsing off", which
// section 5.4 uses to say "you are on the second configured network"
// without spending another LED or asking anyone to count.
LanePattern lanePatternInverted(const LanePattern &pattern);

// Timings, named so the tests and the manual can refer to the same numbers.
constexpr uint16_t kSlowBlinkHalfMs = 500;
constexpr uint16_t kFastBlinkHalfMs = 125;
constexpr uint16_t kCodeFlashMs = 150;
constexpr uint16_t kCodeAwakePeriodMs = 2000;
constexpr uint16_t kCodeAsleepPeriodMs = 10000;
constexpr uint16_t kHeartbeatOnMs = 60;
constexpr uint16_t kHeartbeatAwakePeriodMs = 3000;
constexpr uint16_t kHeartbeatAsleepPeriodMs = 10000;
constexpr uint16_t kBannerFlashMs = 100;
constexpr uint16_t kEventPulseMs = 30;
