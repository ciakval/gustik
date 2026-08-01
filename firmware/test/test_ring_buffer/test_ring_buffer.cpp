#include <unity.h>
#include "transmit/ring_buffer_index.h"
#include "transmit/buffer_capacity.h"

void setUp(void) {}
void tearDown(void) {}

void test_starts_empty(void) {
    RingBufferIndex idx(4);
    TEST_ASSERT_EQUAL_UINT(0, idx.count());
    TEST_ASSERT_FALSE(idx.isFull());
}

void test_push_increments_count_until_full(void) {
    RingBufferIndex idx(3);
    size_t slot0 = idx.push();
    size_t slot1 = idx.push();
    TEST_ASSERT_EQUAL_UINT(0, slot0);
    TEST_ASSERT_EQUAL_UINT(1, slot1);
    TEST_ASSERT_EQUAL_UINT(2, idx.count());
    TEST_ASSERT_FALSE(idx.isFull());
}

void test_becomes_full_at_capacity(void) {
    RingBufferIndex idx(2);
    idx.push();
    idx.push();
    TEST_ASSERT_TRUE(idx.isFull());
    TEST_ASSERT_EQUAL_UINT(2, idx.count());
}

void test_push_beyond_capacity_overwrites_oldest_without_growing_count(void) {
    // AC2 (Story 2.1): buffer reaches capacity and connection is still
    // down -> it starts overwriting the oldest records (best-effort,
    // FR-4), never grows past capacity, never crashes/stops sampling.
    RingBufferIndex idx(2);
    idx.push(); // slot 0
    idx.push(); // slot 1, now full
    size_t overwrittenSlot = idx.push(); // must reuse slot 0 (the oldest)

    TEST_ASSERT_EQUAL_UINT(0, overwrittenSlot);
    TEST_ASSERT_EQUAL_UINT(2, idx.count()); // stays at capacity, never grows
    TEST_ASSERT_TRUE(idx.isFull());
}

void test_oldest_to_newest_returns_logical_order_after_wraparound(void) {
    RingBufferIndex idx(3);
    idx.push(); // slot 0
    idx.push(); // slot 1
    idx.push(); // slot 2, full
    idx.push(); // overwrites slot 0 (oldest)

    // Logical oldest-to-newest order of physical slots is now 1, 2, 0.
    auto order = idx.oldestToNewestSlots();
    TEST_ASSERT_EQUAL_UINT(3, order.size());
    TEST_ASSERT_EQUAL_UINT(1, order[0]);
    TEST_ASSERT_EQUAL_UINT(2, order[1]);
    TEST_ASSERT_EQUAL_UINT(0, order[2]);
}

void test_drain_resets_buffer_to_empty(void) {
    RingBufferIndex idx(2);
    idx.push();
    idx.push();
    idx.drain();
    TEST_ASSERT_EQUAL_UINT(0, idx.count());
    TEST_ASSERT_FALSE(idx.isFull());
}

void test_reset_reinitializes_capacity_and_clears(void) {
    RingBufferIndex idx(2);
    idx.push();
    idx.push();
    idx.reset(5);
    TEST_ASSERT_EQUAL_UINT(5, idx.capacity());
    TEST_ASSERT_EQUAL_UINT(0, idx.count());
}

void test_capacity_covers_at_least_4_hours_at_3_second_interval(void) {
    // NFR-4: buffer capacity >= 4h at the usual sampling frequency.
    size_t capacity = computeBufferCapacityForHours(4.0, 3.0);
    double coveredHours = (capacity * 3.0) / 3600.0;
    TEST_ASSERT_TRUE(coveredHours >= 4.0);
}

void test_capacity_scales_with_sample_interval(void) {
    size_t capacityAt3s = computeBufferCapacityForHours(4.0, 3.0);
    size_t capacityAt5s = computeBufferCapacityForHours(4.0, 5.0);
    // Slower sampling needs fewer slots to cover the same time window.
    TEST_ASSERT_TRUE(capacityAt5s < capacityAt3s);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_starts_empty);
    RUN_TEST(test_push_increments_count_until_full);
    RUN_TEST(test_becomes_full_at_capacity);
    RUN_TEST(test_push_beyond_capacity_overwrites_oldest_without_growing_count);
    RUN_TEST(test_oldest_to_newest_returns_logical_order_after_wraparound);
    RUN_TEST(test_drain_resets_buffer_to_empty);
    RUN_TEST(test_reset_reinitializes_capacity_and_clears);
    RUN_TEST(test_capacity_covers_at_least_4_hours_at_3_second_interval);
    RUN_TEST(test_capacity_scales_with_sample_interval);
    return UNITY_END();
}
