#pragma once

#include <string>

// Parsing and description of the backend's POST /readings response body.
// Pure logic, no Arduino.h - built and unit-tested by [env:native]
// (build_src_filter includes transmit/*.cpp). The hardware half that
// actually performs the request lives in transmit/hw/wifi_client.h.
//
// Why this exists: a 2xx from POST /readings used to say nothing about
// whether anything was actually stored. `client_id` is UNIQUE and the
// backend inserts with ON CONFLICT DO NOTHING (a deliberate backfill-retry
// safety net), so a whole batch can be silently discarded. bug-031 was
// exactly that - the Station printed `sent=yes` every cycle for hours while
// the backend stored nothing, and the firmware had no way to tell. The
// backend now answers `{received, inserted, duplicates, backfilled}` on
// every 2xx; this reads those counts so the Serial diagnostics can say
// "sent but stored nothing" out loud.

struct IngestResponse {
    // False when the body carried no usable counts at all - an error body,
    // an empty body, or an older backend. Counts are then meaningless and
    // must not be reported as zeros, which would look exactly like the
    // "stored nothing" alarm this is meant to raise.
    bool hasCounts = false;
    int received = 0;
    int inserted = 0;
    int duplicates = 0;
    int backfilled = 0;
};

// Deliberately a minimal field scanner rather than a JSON parser: the only
// producer of these bodies is our own backend, the response is a flat object
// of integer fields, and pulling in a JSON library would cost flash the
// ESP32 build does not have to spare (it sits above 90% used).
IngestResponse parseIngestResponse(const std::string &body);

// One-line human summary for the Serial diagnostics. `ok` is the caller's
// 2xx verdict and `statusCode` the raw HTTP status (<=0 when no response was
// received at all).
std::string describeIngestOutcome(bool ok, int statusCode, const IngestResponse &response);
