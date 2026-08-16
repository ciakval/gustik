#include "indicate/panel.h"

namespace {

Lane makeLane(const LanePattern &pattern, unsigned long phase0) {
    Lane lane;
    lane.pattern = pattern;
    lane.phase0 = phase0;
    return lane;
}

int clampOctant(int octant) {
    if (octant < 0) {
        return 0;
    }
    if (octant > 7) {
        return 7;
    }
    return octant;
}

} // namespace

int windDetailPosition(double windSpeedMs) {
    for (int i = 0; i < kDetailPositionCount - 1; i++) {
        if (windSpeedMs < kWindBoundaryMs[i]) {
            return i;
        }
    }
    // Total by construction: NaN falls through every `<` comparison and
    // lands here too, which is the safe end of the scale to land on.
    return kDetailPositionCount - 1;
}

bool windIsStrong(double windSpeedMs) {
    return windSpeedMs >= kWindStrongMs;
}

int signalDetailPosition(bool wifiAssociated, bool rssiValid, int rssiDbm) {
    if (!wifiAssociated || !rssiValid) {
        return kDetailPositionCount - 1;
    }
    for (int i = 0; i < kDetailPositionCount - 1; i++) {
        if (rssiDbm >= kRssiBoundaryDbm[i]) {
            return i;
        }
    }
    return kDetailPositionCount - 1;
}

void StatusPanel::begin(const PanelSettings &settings, unsigned long nowMs) {
    settings_ = settings;
    bootAtMs_ = nowMs;
    modeEnteredAtMs_ = nowMs;
    lastInteractionAtMs_ = nowMs;
    faultSinceMs_ = nowMs;
    // Fire the direction code as soon as wind mode starts displaying.
    directionCodeAtMs_ = nowMs - kDirectionCodeIntervalMs;
}

bool StatusPanel::isSelfTesting(unsigned long nowMs) const {
    return (nowMs - bootAtMs_) < kSelfTestTotalMs;
}

void StatusPanel::setMode(DetailMode mode, unsigned long nowMs) {
    mode_ = mode;
    modeEnteredAtMs_ = nowMs;
    directionCodeAtMs_ = nowMs - kDirectionCodeIntervalMs;
}

void StatusPanel::trackPulses(const PanelInputs &inputs, unsigned long nowMs) {
    if (!havePulseSnapshot_) {
        havePulseSnapshot_ = true;
        lastPulseSnapshot_ = inputs.pulseCountSnapshot;
        return;
    }
    if (inputs.pulseCountSnapshot > lastPulseSnapshot_) {
        lastPulseAtMs_ = nowMs;
        sawPulse_ = true;
    }
    // A decrease is not an event: the sample cycle's readAndResetPulseCount()
    // zeroes the counter every 3 s. Resync silently.
    lastPulseSnapshot_ = inputs.pulseCountSnapshot;
}

PanelOutputs StatusPanel::tick(const PanelInputs &inputs, unsigned long nowMs, ButtonEvent event) {
    PanelOutputs out;  // every lane defaults to off
    if (!settings_.enabled) {
        return out;
    }

    if (event == ButtonEvent::Long) {
        // Hard off is runtime only and never persisted, so a power cycle
        // always comes back with the panel on. That is what keeps "the whole
        // row dark means not running" true in the field despite this
        // deliberately breaking it - see section 5.7.
        hardOff_ = !hardOff_;
        asleep_ = false;
        lastInteractionAtMs_ = nowMs;
    } else if (event == ButtonEvent::Short && !hardOff_) {
        lastInteractionAtMs_ = nowMs;
        if (asleep_) {
            asleep_ = false;  // the press that wakes it does not also advance
        } else {
            int next = (static_cast<int>(mode_) + 1) % kDetailModeCount;
            setMode(static_cast<DetailMode>(next), nowMs);
        }
    }

    // Pulse tracking runs in every mode, so the anemometer's liveness is
    // already current the moment someone switches to sensor mode.
    trackPulses(inputs, nowMs);

    PanelFault fault = highestPriorityFault(inputs);
    if (fault != fault_) {
        fault_ = fault;
        faultSinceMs_ = nowMs;
    }

    if (hardOff_) {
        return out;
    }
    if (isSelfTesting(nowMs)) {
        renderSelfTest(out, nowMs);
        return out;
    }

    if (settings_.sleepTimeoutMs > 0 && (nowMs - lastInteractionAtMs_) >= settings_.sleepTimeoutMs) {
        asleep_ = true;
    }
    if (!asleep_ && mode_ != DetailMode::Wind && (nowMs - modeEnteredAtMs_) >= kModeAutoReturnMs) {
        setMode(DetailMode::Wind, nowMs);
    }

    renderStatusGroup(out, inputs, fault);
    if (!asleep_) {
        renderDetailGroup(out, inputs, nowMs);
    }
    return out;
}

void StatusPanel::renderSelfTest(PanelOutputs &out, unsigned long nowMs) const {
    unsigned long elapsed = nowMs - bootAtMs_;
    if (elapsed < kSelfTestAllOnMs) {
        // All nine on: a dead LED, a cold joint or a swapped jumper is
        // obvious at power-on, and the operator gets a positive "it just
        // started".
        for (int i = 0; i < kStatusLaneCount; i++) {
            out.status[i] = makeLane(lanePatternSolid(), bootAtMs_);
        }
        for (int i = 0; i < kDetailPositionCount; i++) {
            out.detail[i] = makeLane(lanePatternSolid(), bootAtMs_);
        }
        return;
    }
    // Then one sweep left to right across the whole row, which also teaches
    // the row's order without anybody reading anything.
    unsigned long step = (elapsed - kSelfTestAllOnMs) / kSelfTestStepMs;
    if (step < static_cast<unsigned long>(kStatusLaneCount)) {
        out.status[step] = makeLane(lanePatternSolid(), bootAtMs_);
    } else if (step < static_cast<unsigned long>(kStatusLaneCount + kDetailPositionCount)) {
        out.detail[step - kStatusLaneCount] = makeLane(lanePatternSolid(), bootAtMs_);
    }
}

void StatusPanel::renderStatusGroup(PanelOutputs &out, const PanelInputs &inputs, PanelFault fault) const {
    const uint16_t codePeriodMs = asleep_ ? kCodeAsleepPeriodMs : kCodeAwakePeriodMs;
    const uint16_t heartbeatPeriodMs = asleep_ ? kHeartbeatAsleepPeriodMs : kHeartbeatAwakePeriodMs;

    // RED - fault. Only the highest-priority active one; /status.html has
    // the full picture whenever a phone is in reach.
    if (fault == PanelFault::Fatal) {
        out.status[kStatusRed] = makeLane(lanePatternSolid(), faultSinceMs_);
    } else {
        int flashes = faultFlashCount(fault);
        if (flashes > 0) {
            out.status[kStatusRed] = makeLane(lanePatternCode(flashes, codePeriodMs), faultSinceMs_);
        }
    }

    // BLUE - life. Anchored to the last completed sample cycle, so a hung
    // loop() stops re-anchoring it; and since the renderer runs from loop()
    // too, the lane simply freezes - dark, because the pulse is a 60 ms
    // sliver of a 3 s period. Before the first sample the panel anchors to
    // boot instead: the panel ticking at all is already proof of life.
    const unsigned long lifeAnchor = inputs.haveSample ? inputs.lastSampleAtMs : bootAtMs_;
    const LanePattern heartbeat =
        inputs.bufferedCount > 0
            ? lanePatternDoublePulse(kHeartbeatOnMs, kHeartbeatDoubleGapMs, heartbeatPeriodMs)
            : lanePatternPulse(kHeartbeatOnMs, heartbeatPeriodMs);
    out.status[kStatusBlue] = makeLane(heartbeat, lifeAnchor);

    if (asleep_) {
        // Asleep is the fault code and the heartbeat, nothing else - so the
        // panel stops blinking at anybody for eight hours without ever
        // becoming indistinguishable from a dead station.
        return;
    }

    // YELLOW - radio.
    if (inputs.wifiAssociated) {
        const bool weak = inputs.rssiValid && inputs.rssiDbm < kRssiWeakDbm;
        out.status[kStatusYellow] = makeLane(weak ? lanePatternSlowBlink() : lanePatternSolid(), 0);
    }
    // Off when not associated (Q4). The design's "fast blink = scanning" is
    // deliberately not implemented: this firmware connects synchronously
    // inside send(), so there is no scanning state to report and inventing
    // one would be a lie told in LEDs.

    // GREEN - is the data actually landing.
    if (inputs.haveSample) {
        if (inputs.lastSendOk) {
            const bool stored = inputs.hasCounts && inputs.lastInserted > 0;
            // Slow blink covers both "2xx but stored nothing" and "a backend
            // too old to report counts": in neither case do we know the
            // reading is safe, and that is the honest signal.
            out.status[kStatusGreen] = makeLane(stored ? lanePatternSolid() : lanePatternSlowBlink(), 0);
        }
        // Off when the last send failed outright.
    }
}

void StatusPanel::renderDetailGroup(PanelOutputs &out, const PanelInputs &inputs, unsigned long nowMs) {
    // Mode banner (section 5.8): N flashes across the DETAIL GROUP ONLY. A
    // banner across the whole row would look like the low-battery alarm, and
    // the status group must never flash for a reason that is not status.
    const int bannerFlashes = static_cast<int>(mode_) + 1;
    const unsigned long bannerMs = static_cast<unsigned long>(bannerFlashes) * 2UL * kBannerFlashMs;
    if ((nowMs - modeEnteredAtMs_) < bannerMs) {
        const LanePattern banner = lanePatternOneShot(bannerFlashes, kBannerFlashMs, kBannerFlashMs);
        for (int i = 0; i < kDetailPositionCount; i++) {
            out.detail[i] = makeLane(banner, modeEnteredAtMs_);
        }
        return;
    }

    switch (mode_) {
    case DetailMode::Wind:
        renderWind(out, inputs, nowMs);
        break;
    case DetailMode::Signal:
        renderSignal(out, inputs);
        break;
    case DetailMode::Sensors:
        renderSensors(out, inputs);
        break;
    }
}

void StatusPanel::renderWind(PanelOutputs &out, const PanelInputs &inputs, unsigned long nowMs) {
    // Direction, every 5 s: the dot goes dark and status YELLOW blinks
    // octant+1 times (1 = S/sever, 2 = SV, ... 8 = SZ, matching compass.js).
    //
    // This is the ONE place anything touches the status group, and it is a
    // deliberate, bounded exception: direction is the only quantity needing
    // more resolution than five positions, and it matters most exactly when
    // the network is down. The fault lane is never borrowed.
    if ((nowMs - directionCodeAtMs_) >= kDirectionCodeIntervalMs) {
        directionCodeAtMs_ = nowMs;
        directionCodeOctant_ = clampOctant(inputs.windDirOctant);
    }
    const int flashes = directionCodeOctant_ + 1;
    const unsigned long codeMs = static_cast<unsigned long>(flashes) * 2UL * kCodeFlashMs;
    if ((nowMs - directionCodeAtMs_) < codeMs) {
        out.status[kStatusYellow] =
            makeLane(lanePatternOneShot(flashes, kCodeFlashMs, kCodeFlashMs), directionCodeAtMs_);
        return;
    }

    const int position = windDetailPosition(inputs.windSpeedMs);
    out.detail[position] =
        makeLane(windIsStrong(inputs.windSpeedMs) ? lanePatternFastBlink() : lanePatternSolid(), 0);
}

void StatusPanel::renderSignal(PanelOutputs &out, const PanelInputs &inputs) const {
    const int position = signalDetailPosition(inputs.wifiAssociated, inputs.rssiValid, inputs.rssiDbm);
    LanePattern pattern;
    if (position == kDetailPositionCount - 1) {
        // The worst position is the alarm, and the alarm outranks the
        // which-network notch below.
        pattern = lanePatternFastBlink();
    } else if (inputs.networkIndex > 0) {
        pattern = lanePatternInverted(lanePatternPulse(kOtherNetworkNotchMs, kOtherNetworkNotchPeriodMs));
    } else {
        pattern = lanePatternSolid();
    }
    out.detail[position] = makeLane(pattern, 0);
}

void StatusPanel::renderSensors(PanelOutputs &out, const PanelInputs &inputs) const {
    // This mode deliberately breaks the severity metaphor: sensors need three
    // independent indicators, not a scale, so positions are used positionally
    // and their colours carry nothing. Acceptable because it is a diagnostic
    // entered on purpose, never the resting default - and the box legend
    // labels the positions.

    // 1: one short flash per anemometer reed closure, live. Dark while the
    // cups are turning means the anemometer is not wired - bug-059 exactly,
    // which took hours to find and would have been "spin the cups, watch
    // position 1".
    if (sawPulse_) {
        out.detail[0] = makeLane(lanePatternOneShot(1, kEventPulseMs, 0), lastPulseAtMs_);
    }
    // 2: vane - solid inside a plausible ADC band, slow blink outside it.
    out.detail[1] = makeLane(inputs.vaneInRange ? lanePatternSolid() : lanePatternSlowBlink(), 0);
    // 3: magnetometer - solid when the last I2C read succeeded. NOTE this
    // proves the sensor is ALIVE, never that the heading is RIGHT: a
    // magnetometer distorted by the steel base plate passes it perfectly.
    out.detail[2] = makeLane(inputs.magnetometerOk ? lanePatternSolid() : lanePatternSlowBlink(), 0);
    // 4: reserved.
    // 5: the one position whose colour still means what it means everywhere
    // else - dark if all three sensors are fine.
    if (!inputs.vaneInRange || !inputs.magnetometerOk) {
        out.detail[4] = makeLane(lanePatternSlowBlink(), 0);
    }
}
