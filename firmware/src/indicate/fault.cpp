#include "indicate/fault.h"

PanelFault highestPriorityFault(const PanelInputs &inputs) {
    if (inputs.fatal) {
        return PanelFault::Fatal;
    }
    // Code 1 and 2 are answerable from boot: the config is parsed in
    // setup(), and "not associated" is honest during the first seconds.
    if (!inputs.configLoaded) {
        return PanelFault::NoConfig;
    }
    if (!inputs.wifiAssociated) {
        return PanelFault::NoNetwork;
    }
    // Everything below reports on a send that has actually been attempted.
    // Before the first cycle completes, lastSendOk is false and
    // lastHttpStatus is 0 - which would otherwise read as code 3 for the
    // first three seconds of every boot.
    if (!inputs.haveSample) {
        return PanelFault::None;
    }
    if (!inputs.lastSendOk && inputs.lastHttpStatus <= 0) {
        // WiFi is up but no HTTP status came back at all: DNS, route, TLS
        // or timeout. Distinct from Q4, and distinct from a backend that
        // answered and said no.
        return PanelFault::BackendUnreachable;
    }
    if (inputs.lastHttpStatus >= 400 && inputs.lastHttpStatus < 500) {
        return PanelFault::TokenRejected;
    }
    if (inputs.lastSendOk && inputs.hasCounts && inputs.lastInserted == 0) {
        // The bug-031 signature. `hasCounts` is required, so a backend too
        // old to report counts degrades to "no fault" rather than
        // fabricating a permanent code 5 out of a default-zero field -
        // the same guard transmit/ingest_response.h applies for the same
        // reason.
        return PanelFault::StoredNothing;
    }
    if (inputs.bufferedCount > 0) {
        // Usually transient. A persistent 6 with no higher code active
        // means 2/3/4 is intermittent rather than steady.
        return PanelFault::Buffering;
    }
    if (!inputs.clockSynced) {
        return PanelFault::ClockNotSynced;
    }
    if (!inputs.magnetometerOk || !inputs.vaneInRange) {
        // Which sensor is in detail mode "sensors" (section 5.5). The
        // anemometer deliberately cannot raise this: no pulses is a
        // legitimate reading of a calm day, and only a person watching the
        // cups turn can tell it from a broken wire (bug-059).
        return PanelFault::SensorFailing;
    }
    return PanelFault::None;
}

int faultFlashCount(PanelFault fault) {
    int code = static_cast<int>(fault);
    if (code <= 0 || code > 8) {
        return 0;
    }
    return code;
}
