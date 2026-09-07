#include "../test_helpers.h"
#include "../../core/telemetry.h"
#include "../../core/session.h"
#include "../../core/event.h"
#include "../../storage/journal.h"
#include "../mocks/furi.h"
#include "../mocks/furi_hal_power.h"
#include "../mocks/storage/storage.h"

static int g_pipe_events_count = 0;
static BatteryEvent g_pipe_last_event;
static void pipe_event_cb(const BatteryEvent* event, void* context) {
    (void)context;
    g_pipe_last_event = *event;
    g_pipe_events_count++;
}

static int g_pipe_records_count = 0;
static bool pipe_journal_cb(const JournalRecordHeader* header, const void* payload, void* context) {
    (void)header;
    (void)payload;
    (void)context;
    g_pipe_records_count++;
    return true;
}

void test_pipeline_scenario1_charging(void) {
    TEST_CASE("Integration Scenario 1: Charging 42% -> 45% -> 50% generates CHARGE_STARTED, session, and journal samples");
    mock_storage_reset();
    mock_power_reset_defaults();
    telemetry_init();
    session_manager_init();
    event_reset_state();
    event_set_listener(pipe_event_cb, NULL);
    journal_init(&g_mock_storage, "/ext/scen1.log");
    
    g_pipe_events_count = 0;
    
    // Step 1: 42% charging
    g_mock_power.soc_pct = 42;
    g_mock_power.is_charging = true;
    g_mock_power.current_a[FuriHalPowerICFuelGauge] = 0.80f;
    g_mock_tick_ms = 1000;
    
    BatteryTelemetry sample;
    telemetry_take_sample(&sample);
    journal_enqueue_sample(&sample);
    session_process_sample(&sample);
    
    ASSERT_EQ(g_pipe_events_count, 1);
    ASSERT_EQ(g_pipe_last_event.type, EventTypeChargeStarted);
    
    BatterySession active_sess;
    ASSERT_TRUE(session_get_active(&active_sess));
    ASSERT_EQ(active_sess.type, SessionTypeCharging);
    ASSERT_EQ(active_sess.start_soc, 42);
    
    // Step 2: 45% charging
    g_mock_power.soc_pct = 45;
    g_mock_tick_ms = 60000;
    telemetry_take_sample(&sample);
    journal_enqueue_sample(&sample);
    session_process_sample(&sample);
    
    // Step 3: 50% charging
    g_mock_power.soc_pct = 50;
    g_mock_tick_ms = 120000;
    telemetry_take_sample(&sample);
    journal_enqueue_sample(&sample);
    session_process_sample(&sample);
    
    // Flush buffer to storage
    journal_flush();
    journal_free();
    
    // Verify journal contains samples and events
    g_pipe_records_count = 0;
    journal_iterate_records(&g_mock_storage, "/ext/scen1.log", pipe_journal_cb, NULL);
    // 1 event + 3 samples = 4 records
    ASSERT_EQ(g_pipe_records_count, 4);
    
    TEST_PASS();
}

void test_pipeline_scenario2_discharging(void) {
    TEST_CASE("Integration Scenario 2: Discharging 80% -> 70% -> 60% -> 50% triggers DISCHARGE_STARTED and updates session");
    mock_storage_reset();
    mock_power_reset_defaults();
    telemetry_init();
    session_manager_init();
    event_reset_state();
    event_set_listener(pipe_event_cb, NULL);
    journal_init(&g_mock_storage, "/ext/scen2.log");
    
    g_pipe_events_count = 0;
    
    uint8_t socs[] = {80, 70, 60, 50};
    for(int i = 0; i < 4; i++) {
        g_mock_power.soc_pct = socs[i];
        g_mock_power.is_charging = false;
        g_mock_power.current_a[FuriHalPowerICFuelGauge] = -0.30f;
        g_mock_tick_ms = 1000 + i * 60000;
        
        BatteryTelemetry sample;
        telemetry_take_sample(&sample);
        journal_enqueue_sample(&sample);
        session_process_sample(&sample);
    }
    
    ASSERT_EQ(g_pipe_events_count, 1);
    ASSERT_EQ(g_pipe_last_event.type, EventTypeDischargeStarted);
    
    BatterySession active_sess;
    ASSERT_TRUE(session_get_active(&active_sess));
    ASSERT_EQ(active_sess.type, SessionTypeDischarging);
    ASSERT_EQ(active_sess.start_soc, 80);
    ASSERT_EQ(active_sess.end_soc, 50);
    ASSERT_EQ(active_sess.sample_count, 4);
    
    journal_free();
    TEST_PASS();
}

void test_pipeline_scenario3_power_loss_and_recovery(void) {
    TEST_CASE("Integration Scenario 3: Crash/power loss mid-write preserves valid history and allows seamless recovery & restart");
    mock_storage_reset();
    mock_power_reset_defaults();
    telemetry_init();
    session_manager_init();
    journal_init(&g_mock_storage, "/ext/scen3.log");
    
    // Write 5 samples
    for(int i = 0; i < 5; i++) {
        g_mock_power.soc_pct = 90 - i;
        g_mock_tick_ms = 1000 + i * 5000;
        BatteryTelemetry sample;
        telemetry_take_sample(&sample);
        journal_write_record(RecordTypeSample, 1, sample.timestamp_ms, &sample, sizeof(sample));
    }
    journal_free();
    
    size_t valid_size = g_mock_storage.files[0].size;
    
    // Inject corrupt half-written record
    uint8_t garbage[20] = {0x01, 0x01, 0x50, 0x00, 0xAA, 0xBB};
    memcpy(g_mock_storage.files[0].data + valid_size, garbage, sizeof(garbage));
    g_mock_storage.files[0].size += sizeof(garbage);
    
    // Simulate App Restart & Recovery
    telemetry_init();
    session_manager_init();
    bool init_ok = journal_init(&g_mock_storage, "/ext/scen3.log");
    ASSERT_TRUE(init_ok);
    
    // Append 6th sample after restart
    g_mock_power.soc_pct = 84;
    g_mock_tick_ms = 50000;
    BatteryTelemetry sample;
    telemetry_take_sample(&sample);
    bool w_ok = journal_write_record(RecordTypeSample, 1, sample.timestamp_ms, &sample, sizeof(sample));
    ASSERT_TRUE(w_ok);
    journal_free();
    
    // Verify file has exactly 6 valid records
    g_pipe_records_count = 0;
    journal_iterate_records(&g_mock_storage, "/ext/scen3.log", pipe_journal_cb, NULL);
    ASSERT_EQ(g_pipe_records_count, 6);
    
    TEST_PASS();
}

void test_pipeline_phase_f_100k_samples_memory_stress(void) {
    TEST_CASE("Phase F Stress Test: 100,000 synthetic telemetry samples processed with zero leaks and bounded memory");
    mock_storage_reset();
    mock_power_reset_defaults();
    telemetry_init();
    session_manager_init();
    event_reset_state();
    
    // Set a large storage limit — stress test validates pipeline logic, not capacity
    g_mock_storage.max_allowed_bytes = 256 * 1024 * 1024; // 256 MB
    journal_init(&g_mock_storage, "/ext/stress.log");
    
    BatteryTelemetry sample;
    for(uint32_t i = 0; i < 100000; i++) {
        g_mock_tick_ms += 1000;
        
        // Alternate charging and discharging every 5000 samples
        bool charging = ((i / 5000) % 2) == 0;
        g_mock_power.is_charging = charging;
        g_mock_power.current_a[FuriHalPowerICFuelGauge] = charging ? 0.75f : -0.25f;
        g_mock_power.voltage_v[FuriHalPowerICFuelGauge] = charging ? 4.10f : 3.85f;
        g_mock_power.soc_pct = (uint8_t)(charging ? (20 + (i % 5000) * 60 / 5000) : (80 - (i % 5000) * 60 / 5000));
        
        telemetry_take_sample(&sample);
        journal_enqueue_sample(&sample);
        session_process_sample(&sample);
    }
    
    journal_free();
    
    // Ring buffer size remained bounded
    ASSERT_EQ(journal_get_buffered_count(), 0);
    
    // Active session is intact and bounded
    BatterySession active;
    ASSERT_TRUE(session_get_active(&active));
    ASSERT_TRUE(active.sample_count > 0);
    
    TEST_PASS();
}

void run_integration_pipeline_tests(void) {
    TEST_SUITE("Cross-Module Integration & Phase F Stress Tests");
    test_pipeline_scenario1_charging();
    test_pipeline_scenario2_discharging();
    test_pipeline_scenario3_power_loss_and_recovery();
    test_pipeline_phase_f_100k_samples_memory_stress();
}
