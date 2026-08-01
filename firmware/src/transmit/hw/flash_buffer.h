#pragma once

#include <Arduino.h>
#include <vector>
#include "transmit/reading.h"
#include "transmit/ring_buffer_index.h"

// Flash-backed (LittleFS) local buffer for readings that couldn't be sent
// (Story 2.1, FR-4). Slot bookkeeping (overwrite-oldest-when-full policy)
// is delegated entirely to the tested RingBufferIndex; this class only
// does the actual file I/O, which is hardware-coupled and not unit tested.
// One small file per slot ("/buf/<slot>.txt", a simple pipe-delimited
// line - no JSON parser dependency needed on the firmware side).
class FlashBuffer {
public:
    // capacity sized via computeBufferCapacityForHours (>=4h, NFR-4).
    void begin(size_t capacity);

    void push(const Reading &reading);

    // Reads back all buffered readings in oldest-to-newest order (the
    // order Story 2.2's backfill send must use, AD-2) and clears the
    // buffer - callers are responsible for actually sending them first if
    // that matters (best-effort per FR-4, not this class's concern).
    std::vector<Reading> drainAll();

    size_t count() const { return index_.count(); }

private:
    RingBufferIndex index_;

    String slotPath(size_t slot) const;
};
