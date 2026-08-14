#include "transmit/ingest_response.h"

#include <cstddef>

namespace {

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Finds `"<name>": <integer>` and writes the value to `out`.
//
// The quoted name is searched WITH both quotes, so "inserted_total" and
// "notinserted" cannot match "inserted" - a bare substring search would.
bool findIntField(const std::string &body, const char *name, int &out) {
    std::string needle = std::string("\"") + name + "\"";
    std::size_t at = body.find(needle);
    if (at == std::string::npos) {
        return false;
    }

    std::size_t i = at + needle.size();
    while (i < body.size() && isSpace(body[i])) {
        i++;
    }
    if (i >= body.size() || body[i] != ':') {
        return false;
    }
    i++;
    while (i < body.size() && isSpace(body[i])) {
        i++;
    }

    bool negative = false;
    if (i < body.size() && body[i] == '-') {
        negative = true;
        i++;
    }
    if (i >= body.size() || !isDigit(body[i])) {
        return false; // e.g. a string or null value - not a count
    }

    long value = 0;
    while (i < body.size() && isDigit(body[i])) {
        value = value * 10 + (body[i] - '0');
        if (value > 1000000) {
            value = 1000000; // saturate rather than overflow; nothing real gets here
            while (i < body.size() && isDigit(body[i])) {
                i++;
            }
            break;
        }
        i++;
    }

    out = static_cast<int>(negative ? -value : value);
    return true;
}

std::string toString(int value) {
    if (value == 0) {
        return "0";
    }
    bool negative = value < 0;
    long remaining = negative ? -static_cast<long>(value) : value;
    std::string digits;
    while (remaining > 0) {
        digits.insert(digits.begin(), static_cast<char>('0' + (remaining % 10)));
        remaining /= 10;
    }
    return negative ? "-" + digits : digits;
}

} // namespace

IngestResponse parseIngestResponse(const std::string &body) {
    IngestResponse parsed;

    int received = 0;
    int inserted = 0;
    // Both are required: a body carrying only one of them is not the
    // contract we know how to read, and half-parsed counts are worse than
    // none (a phantom inserted=0 reads as the "stored nothing" alarm).
    if (!findIntField(body, "received", received) || !findIntField(body, "inserted", inserted)) {
        return parsed;
    }

    parsed.hasCounts = true;
    parsed.received = received;
    parsed.inserted = inserted;
    // Optional - absent ones stay 0, which is their true value in the
    // contract rather than an unknown.
    findIntField(body, "duplicates", parsed.duplicates);
    findIntField(body, "backfilled", parsed.backfilled);
    return parsed;
}

std::string describeIngestOutcome(bool ok, int statusCode, const IngestResponse &response) {
    if (!ok) {
        return statusCode > 0 ? "send=FAILED http=" + toString(statusCode) : "send=FAILED (no response)";
    }

    std::string summary = "send=ok http=" + toString(statusCode);
    if (!response.hasCounts) {
        return summary + " (no counts in response)";
    }

    summary += " stored=" + toString(response.inserted) + "/" + toString(response.received);
    if (response.backfilled > 0) {
        summary += " backfilled=" + toString(response.backfilled);
    }

    if (response.received > 0 && response.inserted == 0) {
        // The bug-031 alarm: the request succeeded and the backend stored
        // nothing. Almost always a repeated clientId, which on this device
        // has meant a reboot regenerating IDs an earlier boot already sent.
        return summary + " !! BACKEND STORED NOTHING (duplicate clientId)";
    }
    if (response.duplicates > 0) {
        // Expected during a backfill retry - the leading rows of the buffer
        // already landed. Worth printing, not worth alarming about.
        return summary + " (duplicates=" + toString(response.duplicates) + ")";
    }
    return summary;
}
