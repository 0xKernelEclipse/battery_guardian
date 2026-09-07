#include "../test_helpers.h"
#include "../../core/telemetry.h"
#include "../../storage/journal.h"
#include "../mocks/furi.h"
#include "../mocks/storage/storage.h"

void test_buffer_bounded_and_eviction(void) {
    TEST_CASE("Ring buffer is strictly bounded and evicts oldest samples without leaking");
    mock_storage_reset();
    
    // Simulate SD unavailable so buffer fills up and evicts
    g_mock_storage.sd_available = false;
    
    journal_init(&g_mock_storage, "/ext/test.log");
    ASSERT_EQ(journal_get_buffered_count(), 0);
    ASSERT_EQ(journal_get_dropped_count(), 0);
    
    BatteryTelemetry sample;
    memset(&sample, 0, sizeof(BatteryTelemetry));
    
    // Push TELEMETRY_BUFFER_SIZE samples
    for(uint32_t i = 0; i < TELEMETRY_BUFFER_SIZE; i++) {
        sample.timestamp_ms = 1000 + i * 1000;
        journal_enqueue_sample(&sample);
    }
    
    ASSERT_EQ(journal_get_buffered_count(), TELEMETRY_BUFFER_SIZE);
    ASSERT_EQ(journal_get_dropped_count(), 0);
    
    // Push 10 more samples -> should evict 10 oldest
    for(uint32_t i = 0; i < 10; i++) {
        sample.timestamp_ms = 200000 + i * 1000;
        journal_enqueue_sample(&sample);
    }
    
    ASSERT_EQ(journal_get_buffered_count(), TELEMETRY_BUFFER_SIZE);
    ASSERT_EQ(journal_get_dropped_count(), 10);
    
    journal_free();
    TEST_PASS();
}

void run_telemetry_buffer_tests(void) {
    TEST_SUITE("Telemetry Buffer");
    test_buffer_bounded_and_eviction();
}
