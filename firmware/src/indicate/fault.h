#pragma once

#include "indicate/panel_inputs.h"

// Which single fault the status panel's red lane reports (section 5.3 of
// docs/superpowers/specs/2026-08-16-status-led-panel-design.md).
//
// Eight codes is a hard cap, not an accident: counting past eight flashes on
// a moving boat does not work. Anything finer belongs in sensor mode or on
// /status.html, which is reachable whenever a phone is.
enum class PanelFault : int {
    None = 0,
    NoConfig = 1,             // Q3  config.txt missing, unparseable, no networks
    NoNetwork = 2,            // Q4  no configured network in range / assoc failing
    BackendUnreachable = 3,   // Q7  WiFi up, no HTTP status at all (DNS/route/TLS)
    TokenRejected = 4,        // Q8  backend answered 4xx - wrong backend.token
    StoredNothing = 5,        // Q9  2xx but inserted == 0 (the bug-031 signature)
    Buffering = 6,            // Q10 readings queuing to flash
    ClockNotSynced = 7,       // Q11 NTP never succeeded, timestamps unreliable
    SensorFailing = 8,        // Q12-Q14 a sensor is not answering
    Fatal = 100,              // setup() could not complete - solid, not a code
};

// The highest-priority active fault, or None.
//
// PRIORITY IS THE CODE NUMBER, ascending: 1 beats 2 beats 3, and so on. The
// numbering is already ordered by causal depth - no config means the WiFi
// state is meaningless, no WiFi means the backend state is meaningless - so
// the lowest active code is always the one that is worth acting on first.
// Do not reorder without renumbering the manual and the box legend with it.
PanelFault highestPriorityFault(const PanelInputs &inputs);

// The number of flashes for a fault, or 0 for None/Fatal (neither of which
// is rendered as a flash code).
int faultFlashCount(PanelFault fault);
