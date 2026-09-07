#include "../test_helpers.h"
#include "../../core/session.h"
#include "../../core/telemetry.h"
#include "../mocks/furi.h"
#include "../mocks/storage/storage.h"

void test_session_state_machine_transitions(void) {
    TEST_CASE("Session state machine correctly transitions without duplicate sessions");
    session_manager_init();
    
    BatteryTelemetry sample;
    memset(&sample, 0, sizeof(BatteryTelemetry));
    sample.flags = VALID_VOLTAGE | VALID_CURRENT | VALID_TEMP | VALID_SOC | VALID_GAUGE;
    sample.gauge_ok = true;
    sample.voltage_v = 3.80f;
    sample.temperature_c = 25.0f;
    
    // 1. Idle sample -> starts idle session
    sample.timestamp_ms = 1000;
    sample.soc_pct = 50;
    sample.charging = false;
    sample.current_a = 0.0f;
    session_process_sample(&sample);
    
    BatterySession active;
    ASSERT_TRUE(session_get_active(&active));
    ASSERT_EQ(active.type, SessionTypeIdle);
    ASSERT_EQ(active.session_id, 1);
    
    // 2. Charging starts -> ends idle session, starts charging session #2
    sample.timestamp_ms = 2000;
    sample.charging = true;
    sample.current_a = 0.85f;
    sample.voltage_v = 4.05f;
    session_process_sample(&sample);
    
    ASSERT_TRUE(session_get_active(&active));
    ASSERT_EQ(active.type, SessionTypeCharging);
    ASSERT_EQ(active.session_id, 2);
    ASSERT_EQ(active.start_soc, 50);
    
    // 3. Charging continues -> updates session #2 without creating new session
    sample.timestamp_ms = 3000;
    sample.soc_pct = 52;
    session_process_sample(&sample);
    
    ASSERT_TRUE(session_get_active(&active));
    ASSERT_EQ(active.type, SessionTypeCharging);
    ASSERT_EQ(active.session_id, 2);
    ASSERT_EQ(active.sample_count, 2);
    
    // 4. USB disconnected -> starts discharging session #3
    sample.timestamp_ms = 4000;
    sample.charging = false;
    sample.current_a = -0.20f;
    sample.soc_pct = 52;
    session_process_sample(&sample);
    
    ASSERT_TRUE(session_get_active(&active));
    ASSERT_EQ(active.type, SessionTypeDischarging);
    ASSERT_EQ(active.session_id, 3);
    
    TEST_PASS();
}

void test_session_edge_cases(void) {
    TEST_CASE("Session handles one-sample, timestamp regression, and noise gracefully");
    session_manager_init();
    
    BatteryTelemetry sample;
    memset(&sample, 0, sizeof(BatteryTelemetry));
    sample.gauge_ok = true;
    
    // One sample session
    sample.timestamp_ms = 1000;
    sample.charging = true;
    sample.current_a = 0.5f;
    session_process_sample(&sample);
    
    // Rapid state switch
    sample.timestamp_ms = 1050;
    sample.charging = false;
    sample.current_a = -0.1f;
    session_process_sample(&sample);
    
    BatterySession active;
    ASSERT_TRUE(session_get_active(&active));
    ASSERT_EQ(active.type, SessionTypeDischarging);
    
    // Timestamp regression
    sample.timestamp_ms = 900; // time moved backwards
    session_process_sample(&sample);
    ASSERT_TRUE(session_get_active(&active));
    ASSERT_EQ(active.sample_count, 2);
    
    TEST_PASS();
}

void run_session_state_machine_tests(void) {
    TEST_SUITE("Session State Machine");
    test_session_state_machine_transitions();
    test_session_edge_cases();
}
