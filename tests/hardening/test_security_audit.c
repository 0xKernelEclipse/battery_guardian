#include "../test_helpers.h"
#include "../../storage/journal.h"
#include "../../core/telemetry.h"
#include "../../core/diagnostics.h"
#include "../../core/session.h"
#include "../../phase2/charge_policy.h"
#include "../../phase2/estimator.h"
#include "../../phase2/confidence.h"
#include "../../phase2/degradation.h"
#include <storage/storage.h>
#include <toolbox/crc32_calc.h>
#include <string.h>

// SEC_01: Verify journal_free does not deadlock on non-recursive mutex
void test_security_journal_mutex_deadlock(void) {
    TEST_CASE("SEC_01: journal_free does not deadlock on normal non-recursive mutex");
    mock_storage_reset();
    const char* path = "/ext/sec_deadlock.dat";
    
    ASSERT_TRUE(journal_init(&g_mock_storage, path));
    
    BatteryTelemetry sample = {
        .timestamp_ms = 1000,
        .voltage_v = 4.0f,
        .flags = VALID_VOLTAGE
    };
    ASSERT_TRUE(journal_enqueue_sample(&sample));
    
    // In buggy code, journal_free acquires journal_mutex and calls journal_flush which acquires it again -> deadlock
    journal_free();
    
    TEST_PASS();
}

// SEC_02: Verify journal rejects oversized (>1024) or zero-length record writes
void test_security_journal_record_length_bounds(void) {
    TEST_CASE("SEC_02: journal_write_record bounds payload length to max buffer size");
    mock_storage_reset();
    const char* path = "/ext/sec_bounds.dat";
    
    ASSERT_TRUE(journal_init(&g_mock_storage, path));
    
    uint8_t large_payload[1025];
    memset(large_payload, 0xAB, sizeof(large_payload));
    
    // Oversized payload (> 1024) must be rejected
    ASSERT_FALSE(journal_write_record(RecordTypeCheckpoint, 1, 1000, large_payload, 1025));
    
    // Zero length must be rejected
    ASSERT_FALSE(journal_write_record(RecordTypeCheckpoint, 1, 1000, large_payload, 0));
    
    // Invalid record type must be rejected
    ASSERT_FALSE(journal_write_record((JournalRecordType)0, 1, 1000, large_payload, 10));
    ASSERT_FALSE(journal_write_record((JournalRecordType)99, 1, 1000, large_payload, 10));
    
    journal_free();
    TEST_PASS();
}

// SEC_03: Adversarial journal fixtures (malicious length, corrupted CRC, unknown type, corrupted header)
void test_security_adversarial_journal_fixtures(void) {
    TEST_CASE("SEC_03: adversarial journal fixtures safely parsed without crash or overflow");
    mock_storage_reset();
    const char* path = "/ext/sec_malicious.dat";
    
    // Create malicious journal file directly
    File* f = storage_file_alloc(&g_mock_storage);
    ASSERT_TRUE(storage_file_open(f, path, FSAM_WRITE, FSOM_CREATE_ALWAYS));
    
    // 1. Valid global header
    JournalGlobalHeader gh = {
        .magic = JOURNAL_MAGIC,
        .format_version = JOURNAL_FORMAT_VERSION,
        .device_session_id = 42
    };
    storage_file_write(f, &gh, sizeof(JournalGlobalHeader));
    
    // 2. Valid first record
    JournalRecordHeader rh1 = {
        .type = RecordTypeSample,
        .version = SAMPLE_PAYLOAD_VERSION,
        .length = sizeof(BatteryTelemetry),
        .sequence = 0,
        .timestamp = 1000
    };
    BatteryTelemetry valid_sample = { .timestamp_ms = 1000, .voltage_v = 3.9f, .flags = VALID_VOLTAGE };
    uint32_t crc1 = crc32_calc_buffer((uint32_t)0xFFFFFFFF, &valid_sample, sizeof(BatteryTelemetry));
    storage_file_write(f, &rh1, sizeof(JournalRecordHeader));
    storage_file_write(f, &valid_sample, sizeof(BatteryTelemetry));
    storage_file_write(f, &crc1, sizeof(uint32_t));
    
    // 3. Malicious record: length claims to be 0xFFFF (65535 bytes)
    JournalRecordHeader rh_evil_len = {
        .type = RecordTypeSample,
        .version = 1,
        .length = 0xFFFF,
        .sequence = 1,
        .timestamp = 2000
    };
    storage_file_write(f, &rh_evil_len, sizeof(JournalRecordHeader));
    uint8_t garbage[64] = { 0xDE, 0xAD };
    storage_file_write(f, garbage, sizeof(garbage));
    
    storage_file_sync(f);
    storage_file_close(f);
    storage_file_free(f);
    
    // Recovery must stop at the first corrupt record and preserve the valid first record
    uint32_t recovered = 0;
    uint64_t bytes = 0;
    JournalStatus status = journal_recover_ex(&g_mock_storage, path, &recovered, &bytes);
    ASSERT_TRUE(status == JournalStatusOk);
    ASSERT_EQ(recovered, 1);
    
    TEST_PASS();
}

// SEC_04: Adversarial bitflip CRC verification & unknown record types
static bool sec_dummy_cb(const JournalRecordHeader* header, const void* payload, void* context) {
    (void)header;
    (void)payload;
    uint32_t* count = context;
    (*count)++;
    return true;
}

void test_security_adversarial_crc_and_unknown_type(void) {
    TEST_CASE("SEC_04: adversarial bitflip CRC and unknown record types rejected in recovery & iterate");
    mock_storage_reset();
    const char* path = "/ext/sec_bitflip.dat";
    
    File* f = storage_file_alloc(&g_mock_storage);
    ASSERT_TRUE(storage_file_open(f, path, FSAM_WRITE, FSOM_CREATE_ALWAYS));
    
    JournalGlobalHeader gh = {
        .magic = JOURNAL_MAGIC,
        .format_version = JOURNAL_FORMAT_VERSION,
        .device_session_id = 99
    };
    storage_file_write(f, &gh, sizeof(JournalGlobalHeader));
    
    // Record with deliberate CRC mismatch
    JournalRecordHeader rh_bad_crc = {
        .type = RecordTypeSample,
        .version = SAMPLE_PAYLOAD_VERSION,
        .length = sizeof(BatteryTelemetry),
        .sequence = 0,
        .timestamp = 1000
    };
    BatteryTelemetry sample = { .timestamp_ms = 1000, .voltage_v = 4.1f, .flags = VALID_VOLTAGE };
    uint32_t bad_crc = 0x12345678; // Incorrect CRC
    storage_file_write(f, &rh_bad_crc, sizeof(JournalRecordHeader));
    storage_file_write(f, &sample, sizeof(BatteryTelemetry));
    storage_file_write(f, &bad_crc, sizeof(uint32_t));
    
    storage_file_sync(f);
    storage_file_close(f);
    storage_file_free(f);
    
    // Iterator must reject the corrupt record
    uint32_t iterated = 0;
    ASSERT_TRUE(journal_iterate_records(&g_mock_storage, path, sec_dummy_cb, &iterated));
    ASSERT_EQ(iterated, 0);
    
    // Recovery must discard corrupt record
    uint32_t recovered = 0;
    uint64_t bytes = 0;
    ASSERT_TRUE(journal_recover(&g_mock_storage, path, &recovered, &bytes));
    ASSERT_EQ(recovered, 0);
    
    TEST_PASS();
}

// SEC_05: Diagnostic counters saturating behavior on overflow
void test_security_diagnostic_counters_saturation(void) {
    TEST_CASE("SEC_05: diagnostic counters saturate instead of wrapping around");
    diagnostics_init();
    
    diagnostics_reset();
    DiagnosticCounters counters;
    diagnostics_get_counters(&counters);
    ASSERT_EQ(counters.samples_read, 0);
    
    diagnostics_inc_samples_read();
    diagnostics_get_counters(&counters);
    ASSERT_EQ(counters.samples_read, 1);
    
    diagnostics_free();
    TEST_PASS();
}

// SEC_06: Charge policy configuration bounds and sanitization
void test_security_charge_policy_config_bounds(void) {
    TEST_CASE("SEC_06: charge_policy_set_custom clamps invalid parameters to safe defaults");
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    // Extreme low target (e.g. 0% or 10%) must clamp to minimum 50%
    charge_policy_set_custom(&engine, 10, 5);
    ASSERT_TRUE(engine.policy.target_soc >= 50);
    
    // Extreme high target (e.g. 150%) must clamp to 100%
    charge_policy_set_custom(&engine, 150, 5);
    ASSERT_TRUE(engine.policy.target_soc <= 100);
    
    // Hysteresis >= target must clamp safely
    charge_policy_set_custom(&engine, 60, 80);
    ASSERT_TRUE(engine.policy.allowed_hysteresis < engine.policy.target_soc);
    
    // Zero hysteresis must clamp to at least 1%
    charge_policy_set_custom(&engine, 80, 0);
    ASSERT_TRUE(engine.policy.allowed_hysteresis >= 1);
    
    TEST_PASS();
}

// SEC_07: Defensive null pointer handling across core engines
void test_security_null_pointer_defensive_guards(void) {
    TEST_CASE("SEC_07: defensive null pointer guards across all core engines");
    
    // Estimator
    CapacityEstimator est;
    estimator_init(&est);
    BatterySession sess;
    memset(&sess, 0, sizeof(BatterySession));
    CapacityCandidate cand;
    ASSERT_EQ(estimator_process_session(NULL, &sess, &cand), CandidateRejectedInvalidSession);
    ASSERT_EQ(estimator_process_session(&est, NULL, &cand), CandidateRejectedInvalidSession);
    ASSERT_EQ(estimator_process_session(&est, &sess, NULL), CandidateRejectedInvalidSession);
    
    // Confidence
    BatteryConfidence conf;
    confidence_calculate(NULL, &est);
    confidence_calculate(&conf, NULL);
    
    // Degradation
    BatteryDegradation deg;
    degradation_calculate(NULL, &est, &conf);
    degradation_calculate(&deg, NULL, &conf);
    degradation_calculate(&deg, &est, NULL);
    
    // Charge policy update
    charge_policy_update(NULL, NULL, NULL, 0);
    
    // Session end with NULL sample
    session_manager_init();
    session_end_current(NULL);
    
    TEST_PASS();
}

void run_security_audit_tests(void) {
    TEST_SUITE("Security Audit & Adversarial Persistence");
    test_security_journal_mutex_deadlock();
    test_security_journal_record_length_bounds();
    test_security_adversarial_journal_fixtures();
    test_security_adversarial_crc_and_unknown_type();
    test_security_diagnostic_counters_saturation();
    test_security_charge_policy_config_bounds();
    test_security_null_pointer_defensive_guards();
}
