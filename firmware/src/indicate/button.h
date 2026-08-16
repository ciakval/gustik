#pragma once

// Debounce + gesture decoding for the panel's one button (section 6 of
// docs/superpowers/specs/2026-08-16-status-led-panel-design.md).
//
// Pure: fed (level, nowMs) and returning an event, so every gesture is
// host-testable with no hardware. The hardware half (indicate/hw/button_pin)
// does nothing but a digitalRead and the active-LOW inversion.
//
// With no button fitted, INPUT_PULLUP reads "released" forever and this
// decoder emits nothing at all - which is the whole of section 7.3's
// "absent hardware is a no-op" for the button, and is asserted in
// test/test_button.

enum class ButtonEvent : int {
    None = 0,
    Short,  // advance the detail group's mode
    Long,   // toggle hard off
};

// Anything shorter than this is contact bounce, not a press.
constexpr unsigned long kButtonDebounceMs = 30;
// A short press is bounded above so that a press on its way to becoming a
// long press is never also delivered as a short one. The gap between the two
// (800 ms - 2 s) is a deliberate dead zone: releasing in it does nothing,
// which is the safe outcome for an ambiguous gesture.
constexpr unsigned long kButtonShortMaxMs = 800;
constexpr unsigned long kButtonLongMs = 2000;

class ButtonDecoder {
public:
    // `pressed` is the debounced-by-us, already-inverted logical level:
    // true while the button is held down. Call every loop iteration.
    ButtonEvent update(bool pressed, unsigned long nowMs);

    // True once the current press has been held long enough to be a long
    // press, before it is released. Only for a caller that wants to give
    // feedback while holding; the event itself still fires on release.
    bool longPressPending(unsigned long nowMs) const;

private:
    bool stable_ = false;                // last accepted (debounced) level
    bool candidate_ = false;             // level currently being timed
    unsigned long candidateSinceMs_ = 0; // when the raw level last changed
    unsigned long pressedAtMs_ = 0;      // when the accepted press began
    bool initialised_ = false;
    bool suppressRelease_ = false;       // button was already down at boot
};
