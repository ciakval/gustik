#pragma once

#include "indicate/button.h"
#include "indicate/fault.h"
#include "indicate/panel_inputs.h"
#include "indicate/pattern.h"

// The status LED panel's mode machine.
//
// Pure (no Arduino.h): it maps a PanelInputs snapshot plus a clock plus a
// button event onto nine lane patterns, and does nothing else. The hardware
// half (indicate/hw/led_panel) is nine digitalWrite calls.
//
// Full design, including why the panel is nine LEDs in two groups rather
// than four re-purposed ones:
// docs/superpowers/specs/2026-08-16-status-led-panel-design.md
//
//     R   Y   G   B            G   G   Y   Y   R
//    └───── status ─────┘     └────── detail ──────┘
//      colour = identity        colour = severity
//      always on                one dot, button selects what it shows
//
// The two rules the rest of this file exists to keep:
//   * STATUS IS NEVER HIDDEN. Nothing the button does can blank the fault
//     lane, the heartbeat or the data lane. A panel that hides a fault while
//     someone checks the wind is worse than no panel.
//   * COLOUR MEANS ONE THING PER GROUP. In the status group it is identity
//     (yellow = radio); in the detail group it is magnitude (yellow =
//     middling). Section 5.5 (sensors) is the one deliberate exception, and
//     it is a diagnostic entered on purpose, never the resting default.

constexpr int kStatusLaneCount = 4;
constexpr int kDetailPositionCount = 5;

// Indices are physical row order, left to right, for both groups - which is
// also the order the boot self-test sweeps in, so the sweep teaches the row.
enum StatusLane {
    kStatusRed = 0,     // fault
    kStatusYellow = 1,  // radio
    kStatusGreen = 2,   // data landing
    kStatusBlue = 3,    // life
};

struct PanelOutputs {
    Lane status[kStatusLaneCount];
    Lane detail[kDetailPositionCount];
};

// What the detail group is showing. Cycle order is the button's order.
enum class DetailMode : int {
    Wind = 0,     // the default - so the resting panel answers "how windy"
    Signal = 1,
    Sensors = 2,
};
constexpr int kDetailModeCount = 3;

struct PanelSettings {
    bool enabled = true;
    // Time with no button press before the panel sleeps to a heartbeat.
    // 0 = never sleep.
    unsigned long sleepTimeoutMs = 300000;
};

// --- Scale mappings, pure and separately testable -------------------------

// Boundaries shared with backend/src/static/beaufort.js, so the LEDs, the
// dashboard and the Beaufort label never disagree.
constexpr double kWindBoundaryMs[kDetailPositionCount - 1] = {1.6, 3.4, 5.5, 8.0};
// Beaufort 6 - the capsize-risk cue for P550s, and the panel's one genuine
// safety signal.
constexpr double kWindStrongMs = 10.8;

// The same thresholds /status.html already draws reference lines at.
constexpr int kRssiBoundaryDbm[kDetailPositionCount - 1] = {-55, -67, -80, -90};
// Status-lane yellow: solid down to this, slow blink below it. The design's
// table leaves -67..-75 unspecified; it is treated as still good, because a
// lane that blinks at a usable signal trains people to ignore it.
constexpr int kRssiWeakDbm = -75;

// Detail position 0..4 for a wind speed. Total: every input, including
// negative and absurd ones, returns a valid position.
int windDetailPosition(double windSpeedMs);
// Position 5 fast-blinks above Beaufort 6 rather than merely lighting.
bool windIsStrong(double windSpeedMs);
// Detail position 0..4 for a signal strength. Not associated, or no valid
// RSSI, is the worst position.
int signalDetailPosition(bool wifiAssociated, bool rssiValid, int rssiDbm);

// --- Timings --------------------------------------------------------------

constexpr unsigned long kSelfTestAllOnMs = 400;
constexpr unsigned long kSelfTestStepMs = 100;
constexpr unsigned long kSelfTestTotalMs =
    kSelfTestAllOnMs + kSelfTestStepMs * (kStatusLaneCount + kDetailPositionCount);
// A non-default detail mode returns to wind by itself, so the panel is never
// found parked in "signal". A convenience rather than a necessity - with the
// status group always visible, being parked hides nothing important.
constexpr unsigned long kModeAutoReturnMs = 60000;
constexpr unsigned long kDirectionCodeIntervalMs = 5000;
constexpr unsigned long kHeartbeatDoubleGapMs = 120;
// Section 5.4: the signal dot pulses off briefly once a second when we are
// on the second configured network - "pulsing means you're on somebody's
// phone", with nothing to count.
constexpr uint16_t kOtherNetworkNotchMs = 100;
constexpr uint16_t kOtherNetworkNotchPeriodMs = 1000;

class StatusPanel {
public:
    void begin(const PanelSettings &settings, unsigned long nowMs);

    // Call every loop iteration, before the sampling gate - see main.cpp.
    // Cheap, non-blocking, allocation-free.
    PanelOutputs tick(const PanelInputs &inputs, unsigned long nowMs, ButtonEvent event);

    DetailMode mode() const { return mode_; }
    bool isAsleep() const { return asleep_; }
    bool isHardOff() const { return hardOff_; }
    bool isSelfTesting(unsigned long nowMs) const;

private:
    void setMode(DetailMode mode, unsigned long nowMs);
    void trackPulses(const PanelInputs &inputs, unsigned long nowMs);
    void renderSelfTest(PanelOutputs &out, unsigned long nowMs) const;
    void renderStatusGroup(PanelOutputs &out, const PanelInputs &inputs, PanelFault fault) const;
    void renderDetailGroup(PanelOutputs &out, const PanelInputs &inputs, unsigned long nowMs);
    void renderWind(PanelOutputs &out, const PanelInputs &inputs, unsigned long nowMs);
    void renderSignal(PanelOutputs &out, const PanelInputs &inputs) const;
    void renderSensors(PanelOutputs &out, const PanelInputs &inputs) const;

    PanelSettings settings_;
    unsigned long bootAtMs_ = 0;

    DetailMode mode_ = DetailMode::Wind;
    unsigned long modeEnteredAtMs_ = 0;
    unsigned long lastInteractionAtMs_ = 0;
    bool asleep_ = false;
    bool hardOff_ = false;

    // Fault code cycles are anchored to the instant the fault appeared, so
    // the code always starts at flash 1 and is countable from the moment it
    // becomes visible.
    PanelFault fault_ = PanelFault::None;
    unsigned long faultSinceMs_ = 0;

    // Anemometer liveness (section 5.5, detail position 1).
    unsigned long lastPulseSnapshot_ = 0;
    bool havePulseSnapshot_ = false;
    unsigned long lastPulseAtMs_ = 0;
    bool sawPulse_ = false;

    // Wind mode's periodic direction code on the borrowed yellow lane.
    unsigned long directionCodeAtMs_ = 0;
    int directionCodeOctant_ = 0;
};
