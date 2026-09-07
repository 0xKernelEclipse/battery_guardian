#include "../test_helpers.h"
#include "../../storage/journal.h"
#include "../mocks/furi.h"
#include "../mocks/storage/storage.h"

static int g_iterate_count = 0;
static uint32_t g_last_seq = 0;
static uint64_t g_last_ts = 0;

static bool count_and_validate_records_cb(const JournalRecordHeader* header, const void* payload, void* context) {
    (void)payload;
    (void)context;
    
    if(g_iterate_count > 0) {
        CB_ASSERT_TRUE(header->sequence > g_last_seq);
        CB_ASSERT_TRUE(header->timestamp >= g_last_ts);
    }
    
    g_last_seq = header->sequence;
    g_last_ts = header->timestamp;
    g_iterate_count++;
    return true;
}

void test_journal_empty_creation_and_header(void) {
    TEST_CASE("Empty journal creation creates valid global header");
    mock_storage_reset();
    g_mock_tick_ms = 4242;
    
    bool ok = journal_init(&g_mock_storage, "/ext/journal_test.log");
    ASSERT_TRUE(ok);
    
    // File size should be exactly sizeof(JournalGlobalHeader)
    ASSERT_EQ(g_mock_storage.files[0].size, sizeof(JournalGlobalHeader));
    
    JournalGlobalHeader* gh = (JournalGlobalHeader*)g_mock_storage.files[0].data;
    ASSERT_EQ(gh->magic, JOURNAL_MAGIC);
    ASSERT_EQ(gh->format_version, JOURNAL_FORMAT_VERSION);
    ASSERT_EQ(gh->device_session_id, 4242);
    
    journal_free();
    TEST_PASS();
}

void test_journal_append_and_sequence(void) {
    TEST_CASE("Appending multiple records increments sequence and tracks timestamps");
    mock_storage_reset();
    journal_init(&g_mock_storage, "/ext/journal_test.log");
    
    char payload1[] = "Telemetry snapshot 1";
    char payload2[] = "Telemetry snapshot 2";
    char payload3[] = "Telemetry snapshot 3";
    
    bool w1 = journal_write_record(RecordTypeEvent, 1, 1000, payload1, sizeof(payload1));
    bool w2 = journal_write_record(RecordTypeEvent, 1, 2000, payload2, sizeof(payload2));
    bool w3 = journal_write_record(RecordTypeEvent, 1, 3000, payload3, sizeof(payload3));
    
    ASSERT_TRUE(w1);
    ASSERT_TRUE(w2);
    ASSERT_TRUE(w3);
    
    journal_free();
    
    // Iterate and verify sequence and timestamps
    g_iterate_count = 0;
    g_last_seq = 0;
    g_last_ts = 0;
    
    bool it_ok = journal_iterate_records(&g_mock_storage, "/ext/journal_test.log", count_and_validate_records_cb, NULL);
    ASSERT_TRUE(it_ok);
    ASSERT_EQ(g_iterate_count, 3);
    
    TEST_PASS();
}

void run_journal_record_tests(void) {
    TEST_SUITE("Journal Records");
    test_journal_empty_creation_and_header();
    test_journal_append_and_sequence();
}
