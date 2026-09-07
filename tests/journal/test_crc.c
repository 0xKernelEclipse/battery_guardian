#include "../test_helpers.h"
#include "../../storage/journal.h"
#include "../mocks/furi.h"
#include "../mocks/storage/storage.h"

static int g_valid_count = 0;
static bool dummy_cb(const JournalRecordHeader* header, const void* payload, void* context) {
    (void)header;
    (void)payload;
    (void)context;
    g_valid_count++;
    return true;
}

void test_journal_crc_corrupted_payload(void) {
    TEST_CASE("Corrupted payload is detected and rejected by CRC");
    mock_storage_reset();
    journal_init(&g_mock_storage, "/ext/crc_test.log");
    
    char payload1[] = "Hello World 1";
    char payload2[] = "Hello World 2";
    
    journal_write_record(RecordTypeEvent, 1, 1000, payload1, sizeof(payload1));
    journal_write_record(RecordTypeEvent, 1, 2000, payload2, sizeof(payload2));
    journal_free();
    
    // Corrupt payload of record 1: flip a bit in payload area
    // Offset: sizeof(JournalGlobalHeader) + sizeof(JournalRecordHeader) + 2
    size_t corrupt_pos = sizeof(JournalGlobalHeader) + sizeof(JournalRecordHeader) + 2;
    g_mock_storage.files[0].data[corrupt_pos] ^= 0xFF;
    
    g_valid_count = 0;
    journal_iterate_records(&g_mock_storage, "/ext/crc_test.log", dummy_cb, NULL);
    
    // First record should fail CRC, stopping iteration immediately
    ASSERT_EQ(g_valid_count, 0);
    
    TEST_PASS();
}

void test_journal_crc_corrupted_crc_field(void) {
    TEST_CASE("Corrupted CRC field stops parser immediately");
    mock_storage_reset();
    journal_init(&g_mock_storage, "/ext/crc_test2.log");
    
    char payload1[] = "Hello World 1";
    char payload2[] = "Hello World 2";
    
    journal_write_record(RecordTypeEvent, 1, 1000, payload1, sizeof(payload1));
    journal_write_record(RecordTypeEvent, 1, 2000, payload2, sizeof(payload2));
    journal_free();
    
    // Corrupt CRC of record 1
    // Offset: sizeof(JournalGlobalHeader) + sizeof(JournalRecordHeader) + sizeof(payload1)
    size_t crc_pos = sizeof(JournalGlobalHeader) + sizeof(JournalRecordHeader) + sizeof(payload1);
    g_mock_storage.files[0].data[crc_pos] ^= 0x55;
    
    g_valid_count = 0;
    journal_iterate_records(&g_mock_storage, "/ext/crc_test2.log", dummy_cb, NULL);
    ASSERT_EQ(g_valid_count, 0);
    
    TEST_PASS();
}

void run_journal_crc_tests(void) {
    TEST_SUITE("Journal CRC Verification");
    test_journal_crc_corrupted_payload();
    test_journal_crc_corrupted_crc_field();
}
