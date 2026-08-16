#include "indicate/button.h"

ButtonEvent ButtonDecoder::update(bool pressed, unsigned long nowMs) {
    if (!initialised_) {
        // Adopt whatever the pin reads at boot without emitting an event -
        // a button held down through reset must not fire on release, and a
        // floating-then-settling pin must not fire at all.
        initialised_ = true;
        stable_ = pressed;
        candidate_ = pressed;
        candidateSinceMs_ = nowMs;
        pressedAtMs_ = nowMs;
        suppressRelease_ = pressed;
        return ButtonEvent::None;
    }

    if (pressed != candidate_) {
        candidate_ = pressed;
        candidateSinceMs_ = nowMs;
        return ButtonEvent::None;
    }
    if (candidate_ == stable_) {
        return ButtonEvent::None;
    }
    if (nowMs - candidateSinceMs_ < kButtonDebounceMs) {
        return ButtonEvent::None;  // not stable long enough yet
    }

    // The candidate level has been steady for the debounce window: accept
    // it. Time the press from the transition itself, not from the moment it
    // was accepted, so the debounce window does not eat into the measured
    // hold duration.
    stable_ = candidate_;
    if (stable_) {
        pressedAtMs_ = candidateSinceMs_;
        return ButtonEvent::None;  // gestures are decided on release
    }

    if (suppressRelease_) {
        // The button was already down at the first update() - held through
        // reset, or a floating pin that had not settled. Its release is not
        // a gesture anybody made.
        suppressRelease_ = false;
        return ButtonEvent::None;
    }

    unsigned long heldMs = candidateSinceMs_ - pressedAtMs_;
    if (heldMs >= kButtonLongMs) {
        return ButtonEvent::Long;
    }
    if (heldMs <= kButtonShortMaxMs) {
        // heldMs >= kButtonDebounceMs is guaranteed by construction: both
        // edges had to be stable for the debounce window to be accepted.
        return ButtonEvent::Short;
    }
    return ButtonEvent::None;  // the dead zone - see button.h
}

bool ButtonDecoder::longPressPending(unsigned long nowMs) const {
    return stable_ && (nowMs - pressedAtMs_) >= kButtonLongMs;
}
