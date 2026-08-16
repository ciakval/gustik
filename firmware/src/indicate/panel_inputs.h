#pragma once

// The one-way snapshot the status LED panel reads.
//
// INVARIANT (constraint C3 of the design): nothing downstream of PanelInputs
// may write to anything upstream of it. The panel may not delay a sample,
// clear a buffer, touch WiFi or re-read a sensor - it only ever reads this
// struct. That is what makes "the panel is optional" a fact rather than a
// hope, and it is the thing to check in review of any change here.
//
// Every field is already computed by main.cpp's loop() for its own Serial
// diagnostics; the snapshot exists so the panel cannot reach for anything
// else. Pure (no Arduino.h), so the whole mode machine is host-testable.
struct PanelInputs {
    // --- Q1/Q2: is it running, is the loop alive -------------------------
    // millis() at the end of the most recent completed sample cycle. The
    // heartbeat pulse is anchored to it, so a hung loop() stops re-anchoring
    // it - and since the panel is rendered from loop() too, the blue lane
    // freezes wherever it was, which is dark ~98% of the time. "Blue dark,
    // other lanes lit" is exactly the bug-030 signature, which otherwise
    // produced no symptom at all.
    unsigned long lastSampleAtMs = 0;
    bool haveSample = false;  // false until the first cycle completes

    // Reserved: setup() could not complete at all (red lane solid, not a
    // flash code). No path in this firmware currently sets it - setup() has
    // no failure mode that still reaches loop(), and a setup() that hangs
    // never renders the panel at all. Kept because the encoding needs a
    // "fatal" state and a future guard may gain one.
    bool fatal = false;

    // --- Q3: config ------------------------------------------------------
    bool configLoaded = false;  // config.txt parsed and named >=1 network

    // --- Q4/Q5/Q6: radio -------------------------------------------------
    bool wifiAssociated = false;
    bool rssiValid = false;
    int rssiDbm = 0;
    // Index into StationConfig::networks of the network actually associated,
    // or -1 when unknown. Only used to distinguish "shore wifi" from
    // "somebody's phone hotspot" (section 5.4).
    int networkIndex = -1;

    // --- Q7/Q8/Q9/Q10: is the data landing -------------------------------
    bool lastSendOk = false;    // the caller's 2xx verdict
    int lastHttpStatus = 0;     // <=0 when no response arrived at all
    bool hasCounts = false;     // the response carried {received, inserted, ...}
    int lastInserted = 0;       // rows the backend says it actually stored
    unsigned long bufferedCount = 0;

    // --- Q11: clock ------------------------------------------------------
    bool clockSynced = false;

    // --- Q12/Q13/Q14: sensors --------------------------------------------
    bool magnetometerOk = false;
    bool vaneInRange = false;
    // Free-running, non-resetting anemometer pulse count. The panel only
    // compares successive snapshots and flashes on an increase; a decrease
    // (which happens every time the sample cycle calls
    // readAndResetPulseCount()) is treated as a resync, not an event.
    unsigned long pulseCountSnapshot = 0;

    // --- Q15: what is the wind doing -------------------------------------
    double windSpeedMs = 0.0;
    int windDirOctant = 0;
};
