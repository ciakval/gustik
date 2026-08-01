#pragma once

#include <cstddef>
#include <vector>

// Slot-index bookkeeping for a fixed-capacity ring buffer. Pure logic - no
// knowledge of what's actually stored in each slot or how (that's the
// hardware/flash-backed layer's job, transmit/hw/flash_buffer.*). Overwrites
// the oldest slot once full (best-effort, FR-4) instead of growing or
// rejecting new pushes.
class RingBufferIndex {
public:
    RingBufferIndex() : capacity_(1) {}
    explicit RingBufferIndex(size_t capacity) : capacity_(capacity) {}

    // Reinitializes to a new capacity, fully empty. Needed by callers (e.g.
    // FlashBuffer) that can't know the real capacity until begin()-time.
    void reset(size_t capacity) {
        capacity_ = capacity;
        drain();
    }

    // Returns the physical slot index the caller should write the new
    // record into (reusing the oldest slot once full).
    size_t push() {
        size_t slot = writeIndex_;
        writeIndex_ = (writeIndex_ + 1) % capacity_;
        if (count_ < capacity_) {
            count_++;
        } else {
            oldestIndex_ = (oldestIndex_ + 1) % capacity_;
        }
        return slot;
    }

    // Physical slot indices in oldest-to-newest logical order - what a
    // backfill send (Story 2.2) needs to transmit in the right order.
    std::vector<size_t> oldestToNewestSlots() const {
        std::vector<size_t> result;
        result.reserve(count_);
        for (size_t i = 0; i < count_; i++) {
            result.push_back((oldestIndex_ + i) % capacity_);
        }
        return result;
    }

    void drain() {
        count_ = 0;
        writeIndex_ = 0;
        oldestIndex_ = 0;
    }

    size_t count() const { return count_; }
    bool isFull() const { return count_ == capacity_; }
    size_t capacity() const { return capacity_; }

private:
    size_t capacity_;
    size_t count_ = 0;
    size_t writeIndex_ = 0;
    size_t oldestIndex_ = 0;
};
