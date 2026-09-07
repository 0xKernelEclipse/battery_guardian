#include "../test_helpers.h"
#include "../../storage/journal.h"
#include "../../core/telemetry.h"
#include <storage/storage.h>
#include <toolbox/crc32_calc.h>
#include <string.h>

void test_journal_truncation_over_64kb(void) {
    TEST_CASE("Journal recovery handles files >64KB without 16-bit truncation");
    mock_storage_reset();
    const char* path = "/ext/journal_large.dat";
    
    // Initialize journal
    ASSERT_TRUE(journal_init(&g_mock_storage, path));
    
    // Write records to exceed 64KB (e.g., 70,000 bytes)
    BatteryTelemetry sample;
    memset(&sample, 0, sizeof(BatteryTelemetry));
    sample.voltage_v = 3.82f;
    sample.current_a = -0.15f;
    sample.temperature_c = 25.0f;
    sample.soc_pct = 75;
    sample.flags = VALID_VOLTAGE | VALID_CURRENT | VALID_TEMP | VALID_SOC;
    
    uint32_t records_written = 0;
    for(uint32_t i = 0; i < 950; i++) {
        sample.timestamp_ms = 1000 + i * 1000;
        if(journal_write_record(RecordTypeSample, 1, sample.timestamp_ms, &sample, sizeof(BatteryTelemetry))) {
            records_written++;
        }
    }
    
    journal_free();
    
    // Verify file is indeed > 64KB (65536 bytes)
    File* f = storage_file_alloc(&g_mock_storage);
    ASSERT_TRUE(storage_file_open(f, path, FSAM_READ | FSAM_WRITE, FSOM_OPEN_EXISTING));
    uint64_t valid_size = storage_file_size(f);
    ASSERT_TRUE(valid_size > 65536);
    
    // Append 50 bytes of corrupted garbage at the end
    uint8_t garbage[50];
    memset(garbage, 0xAA, sizeof(garbage));
    storage_file_seek(f, (uint32_t)valid_size, true);
    storage_file_write(f, garbage, sizeof(garbage));
    storage_file_sync(f);
    uint64_t corrupted_size = storage_file_size(f);
    ASSERT_TRUE(corrupted_size == valid_size + 50);
    storage_file_close(f);
    storage_file_free(f);
    
    // Run recovery on the >64KB corrupted file
    uint32_t recovered_records = 0;
    uint64_t recovered_bytes = 0;
    JournalStatus status = journal_recover_ex(&g_mock_storage, path, &recovered_records, &recovered_bytes);
    
    ASSERT_TRUE(status == JournalStatusOk);
    ASSERT_EQ(recovered_records, records_written);
    ASSERT_TRUE(recovered_bytes == valid_size);
    
    // Verify truncated file matches valid size (garbage removed)
    f = storage_file_alloc(&g_mock_storage);
    ASSERT_TRUE(storage_file_open(f, path, FSAM_READ, FSOM_OPEN_EXISTING));
    ASSERT_TRUE(storage_file_size(f) == valid_size);
    storage_file_close(f);
    storage_file_free(f);
    
    TEST_PASS();
}

void test_journal_storage_full_quota(void) {
    TEST_CASE("Journal strictly caps file at JOURNAL_MAX_FILE_SIZE (512KB)");
    mock_storage_reset();
    const char* path = "/ext/journal_quota.dat";
    
    ASSERT_TRUE(journal_init(&g_mock_storage, path));
    
    uint8_t payload[500];
    memset(payload, 0x55, sizeof(payload));
    
    uint32_t accepted = 0;
    uint32_t rejected = 0;
    
    for(uint32_t i = 0; i < 1100; i++) {
        if(journal_write_record(RecordTypeCheckpoint, 1, 1000 + i, payload, sizeof(payload))) {
            accepted++;
        } else {
            rejected++;
        }
    }
    
    ASSERT_TRUE(accepted > 0);
    ASSERT_TRUE(rejected > 0); // Quota must have triggered!
    
    journal_free();
    
    // Verify file size does not exceed JOURNAL_MAX_FILE_SIZE
    File* f = storage_file_alloc(&g_mock_storage);
    ASSERT_TRUE(storage_file_open(f, path, FSAM_READ, FSOM_OPEN_EXISTING));
    uint64_t final_size = storage_file_size(f);
    ASSERT_TRUE(final_size <= JOURNAL_MAX_FILE_SIZE);
    storage_file_close(f);
    storage_file_free(f);
    
    // Verify journal file is still 100% valid and recoverable
    uint32_t rec_count = 0;
    uint64_t rec_bytes = 0;
    ASSERT_TRUE(journal_recover(&g_mock_storage, path, &rec_count, &rec_bytes));
    ASSERT_EQ(rec_count, accepted);
    
    TEST_PASS();
}

void test_journal_version_mismatch_rejection(void) {
    TEST_CASE("Journal rejects unsupported future format version safely");
    mock_storage_reset();
    const char* path = "/ext/journal_future.dat";
    
    // Manually create a journal file with format_version = 99 (future version)
    File* f = storage_file_alloc(&g_mock_storage);
    ASSERT_TRUE(storage_file_open(f, path, FSAM_WRITE, FSOM_CREATE_ALWAYS));
    
    JournalGlobalHeader gh;
    gh.magic = JOURNAL_MAGIC;
    gh.format_version = 99; // Future version
    gh.device_session_id = 12345;
    storage_file_write(f, &gh, sizeof(JournalGlobalHeader));
    
    // Write 100 bytes of dummy data
    uint8_t dummy[100];
    memset(dummy, 0x42, sizeof(dummy));
    storage_file_write(f, dummy, sizeof(dummy));
    storage_file_sync(f);
    uint64_t original_size = storage_file_size(f);
    storage_file_close(f);
    storage_file_free(f);
    
    // Call journal_recover_ex: must reject version mismatch
    uint32_t rec_count = 0;
    uint64_t rec_bytes = 0;
    JournalStatus status = journal_recover_ex(&g_mock_storage, path, &rec_count, &rec_bytes);
    ASSERT_TRUE(status == JournalStatusErrorVersionMismatch);
    
    // Verify the file was NOT truncated or overwritten!
    f = storage_file_alloc(&g_mock_storage);
    ASSERT_TRUE(storage_file_open(f, path, FSAM_READ, FSOM_OPEN_EXISTING));
    ASSERT_TRUE(storage_file_size(f) == original_size);
    storage_file_close(f);
    storage_file_free(f);
    
    // Call journal_init: must safely fail and refuse to corrupt future data
    ASSERT_FALSE(journal_init(&g_mock_storage, path));
    
    TEST_PASS();
}

void run_journal_hardening_tests(void) {
    TEST_SUITE("Journal Hardening & Quota Protection");
    test_journal_truncation_over_64kb();
    test_journal_storage_full_quota();
    test_journal_version_mismatch_rejection();
}
