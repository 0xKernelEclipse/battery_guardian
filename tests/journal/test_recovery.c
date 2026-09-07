#include "../test_helpers.h"
#include "../../storage/journal.h"
#include "../mocks/furi.h"
#include "../mocks/storage/storage.h"

static int g_fuzz_valid_records = 0;
static bool fuzz_cb(const JournalRecordHeader* header, const void* payload, void* context) {
    (void)header;
    (void)payload;
    (void)context;
    g_fuzz_valid_records++;
    return true;
}

void test_journal_recovery_prefix_and_append(void) {
    TEST_CASE("Journal recovery preserves valid prefix and allows new appends");
    mock_storage_reset();
    journal_init(&g_mock_storage, "/ext/recovery.log");
    
    char p1[] = "Valid Record 1";
    char p2[] = "Valid Record 2";
    char p3[] = "Valid Record 3";
    
    journal_write_record(RecordTypeEvent, 1, 1000, p1, sizeof(p1));
    journal_write_record(RecordTypeEvent, 1, 2000, p2, sizeof(p2));
    journal_write_record(RecordTypeEvent, 1, 3000, p3, sizeof(p3));
    journal_free();
    
    size_t full_size = g_mock_storage.files[0].size;
    
    // Simulate abrupt power loss in the middle of writing a 4th record:
    // Append partial header + garbage bytes
    uint8_t garbage[12] = {0x03, 0x01, 0xFF, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11};
    memcpy(g_mock_storage.files[0].data + full_size, garbage, sizeof(garbage));
    g_mock_storage.files[0].size += sizeof(garbage);
    
    // Perform recovery
    uint32_t recovered_records = 0;
    uint64_t recovered_bytes = 0;
    bool rec_ok = journal_recover(&g_mock_storage, "/ext/recovery.log", &recovered_records, &recovered_bytes);
    
    ASSERT_TRUE(rec_ok);
    ASSERT_EQ(recovered_records, 3);
    ASSERT_EQ(recovered_bytes, full_size);
    ASSERT_EQ(g_mock_storage.files[0].size, full_size); // File truncated back to valid prefix
    
    // Re-init journal (reopening recovered journal) and append 4th record
    journal_init(&g_mock_storage, "/ext/recovery.log");
    char p4[] = "Valid Record 4";
    bool w4 = journal_write_record(RecordTypeEvent, 1, 4000, p4, sizeof(p4));
    ASSERT_TRUE(w4);
    journal_free();
    
    g_fuzz_valid_records = 0;
    journal_iterate_records(&g_mock_storage, "/ext/recovery.log", fuzz_cb, NULL);
    ASSERT_EQ(g_fuzz_valid_records, 4);
    
    TEST_PASS();
}

void test_journal_power_loss_fuzz(void) {
    TEST_CASE("Journal Power-Loss Fuzz: Truncating at every byte offset preserves valid prefix");
    
    // 1. Create a reference journal with 5 valid records
    mock_storage_reset();
    journal_init(&g_mock_storage, "/ext/fuzz_master.log");
    for(int i = 0; i < 5; i++) {
        char payload[32];
        snprintf(payload, sizeof(payload), "Sample Telemetry Payload %d", i);
        journal_write_record(RecordTypeSample, 1, 1000 + i * 1000, payload, (uint16_t)(strlen(payload) + 1));
    }
    journal_free();
    
    size_t master_size = g_mock_storage.files[0].size;
    uint8_t* master_data = (uint8_t*)malloc(master_size);
    memcpy(master_data, g_mock_storage.files[0].data, master_size);
    
    // Calculate record byte offsets in the master journal
    // Header size + 5 records
    // Each record = sizeof(JournalRecordHeader) + len + sizeof(uint32_t)
    size_t rec_len = sizeof(JournalRecordHeader) + (strlen("Sample Telemetry Payload 0") + 1) + sizeof(uint32_t);
    
    // 2. Truncate at every single byte offset from 0 to master_size
    for(size_t cut = 0; cut <= master_size; cut++) {
        mock_storage_reset();
        
        // Populate truncated file
        g_mock_storage.files[0].in_use = true;
        strncpy(g_mock_storage.files[0].name, "/ext/fuzz_cut.log", sizeof(g_mock_storage.files[0].name));
        g_mock_storage.files[0].capacity = master_size + 128;
        g_mock_storage.files[0].data = (uint8_t*)malloc(g_mock_storage.files[0].capacity);
        memcpy(g_mock_storage.files[0].data, master_data, cut);
        g_mock_storage.files[0].size = cut;
        
        // Recover truncated file
        uint32_t rec_count = 0;
        uint64_t rec_bytes = 0;
        journal_recover(&g_mock_storage, "/ext/fuzz_cut.log", &rec_count, &rec_bytes);
        
        // Expected valid records based on cut offset:
        size_t expected_records = 0;
        if(cut >= sizeof(JournalGlobalHeader)) {
            size_t available_for_records = cut - sizeof(JournalGlobalHeader);
            expected_records = available_for_records / rec_len;
        }
        
        ASSERT_EQ(rec_count, expected_records);
        
        // Iterate recovered file to guarantee parser never crashes or returns corrupted record
        g_fuzz_valid_records = 0;
        journal_iterate_records(&g_mock_storage, "/ext/fuzz_cut.log", fuzz_cb, NULL);
        ASSERT_EQ((size_t)g_fuzz_valid_records, expected_records);
    }
    
    free(master_data);
    TEST_PASS();
}

void run_journal_recovery_tests(void) {
    TEST_SUITE("Journal Recovery & Power-Loss Fuzz");
    test_journal_recovery_prefix_and_append();
    test_journal_power_loss_fuzz();
}
