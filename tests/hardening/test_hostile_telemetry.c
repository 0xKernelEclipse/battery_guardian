#include "../test_helpers.h"
#include <furi.h>
#include "../../core/telemetry.h"
#include "../../core/session.h"
#include "../../phase2/battery_health.h"
#include "../../phase2/estimator.h"
#include "../../phase2/degradation.h"
#include <string.h>
#include <math.h>

void test_hostile_divide_by_zero_guards(void) {
    TEST_CASE("Divide-by-zero numerical guards in health, degradation, and estimator");
    // 1. Health Engine with 0 or negative reference capacity
    BatteryHealthEngine* engine = battery_health_alloc();
    engine->reference_capacity_mah = 0.0f; // Hostile input
    engine->has_estimate = true;
    engine->estimator.robust_estimate_mah = 1800.0f;
    
    BatteryHealthSnapshot snap;
    battery_health_get_snapshot(engine, 10000, &snap);
    ASSERT_EQ(snap.observed_health_pct, 0); // Must be safe 0, not NaN/Inf!
    ASSERT_TRUE(!isnan((double)snap.observed_health_pct));
    
    engine->reference_capacity_mah = -500.0f; // Hostile negative
    battery_health_get_snapshot(engine, 10000, &snap);
    ASSERT_EQ(snap.observed_health_pct, 0);
    battery_health_free(engine);
    
    // 2. Degradation calculation with identical points (zero variance denominator)
    CapacityEstimator est;
    estimator_init(&est);
    for(int i = 0; i < 6; i++) {
        est.accepted[i].candidate_capacity_mah = 2000.0f;
    }
    est.accepted_count = 6;
    est.head = 6;
    est.robust_estimate_mah = 0.0f; // Hostile 0 base capacity
    
    BatteryConfidence conf;
    memset(&conf, 0, sizeof(BatteryConfidence));
    conf.overall = ConfidenceLevelHigh;
    
    BatteryDegradation deg;
    degradation_calculate(&deg, &est, &conf);
    ASSERT_TRUE(!isnan(deg.capacity_slope));
    ASSERT_TRUE(!isnan(deg.percent_per_100_cycles));
    ASSERT_TRUE(!isinf(deg.percent_per_100_cycles));
    
    // 3. Estimator process session with median = 0.0f
    est.accepted_count = 4;
    for(int i = 0; i < 4; i++) {
        est.accepted[i].candidate_capacity_mah = 0.0f;
    }
    est.robust_estimate_mah = 0.0f;
    
    BatterySession sess;
    memset(&sess, 0, sizeof(BatterySession));
    sess.type = SessionTypeDischarging;
    sess.start_timestamp = 1000;
    sess.end_timestamp = 2000;
    sess.sample_count = 50;
    sess.start_soc = 90;
    sess.end_soc = 60;
    sess.accumulated_energy_mah = 600.0f;
    sess.quality_flags = SESSION_QUALITY_ENOUGH_SAMPLES |
                         SESSION_QUALITY_MEANINGFUL_SOC |
                         SESSION_QUALITY_NOT_INTERRUPTED |
                         SESSION_QUALITY_NO_SENSOR_FAULT |
                         SESSION_QUALITY_NO_TEMP_EXCURSION;
                         
    CapacityCandidate cand;
    CandidateRejectionReason reason = estimator_process_session(&est, &sess, &cand);
    ASSERT_TRUE(reason == CandidateRejectedNone || reason == CandidateRejectedOutlier);
    
    TEST_PASS();
}

void test_hostile_telemetry_extreme_values(void) {
    TEST_CASE("Hostile telemetry extreme values & boundary rejection");
    telemetry_init();
    session_manager_init();
    
    // Test extreme values fed into session processor
    BatteryTelemetry sample;
    memset(&sample, 0, sizeof(BatteryTelemetry));
    sample.timestamp_ms = 1000;
    sample.voltage_v = -999.0f; // Impossible
    sample.current_a = 5000.0f;  // Impossible
    sample.temperature_c = 1000.0f; // Extreme temperature excursion
    sample.soc_pct = 250; // Impossible
    sample.charging = true;
    
    session_process_sample(&sample);
    BatterySession active;
    ASSERT_TRUE(session_get_active(&active));
    
    // Second sample triggers temp excursion check (>45C)
    sample.timestamp_ms = 2000;
    session_process_sample(&sample);
    ASSERT_TRUE(session_get_active(&active));
    // Quality flag for NO_TEMP_EXCURSION must be stripped
    ASSERT_TRUE((active.quality_flags & SESSION_QUALITY_NO_TEMP_EXCURSION) == 0);
    
    // Estimator must reject this candidate session due to temperature excursion
    CapacityEstimator est;
    estimator_init(&est);
    CapacityCandidate cand;
    CandidateRejectionReason reason = estimator_process_session(&est, &active, &cand);
    ASSERT_TRUE(reason == CandidateRejectedNoisyData || reason == CandidateRejectedInvalidSession);
    
    TEST_PASS();
}

void test_monotonic_32bit_tick_rollover(void) {
    TEST_CASE("Monotonic 64-bit timestamp tracking across 32-bit tick rollover");
    telemetry_init();
    
    // Mock tick starting just before 32-bit rollover
    g_mock_tick_ms = 0xFFFFFFF0; // 16 ticks before wrap
    
    BatteryTelemetry s1;
    ASSERT_TRUE(telemetry_take_sample(&s1));
    ASSERT_TRUE(s1.timestamp_ms == 0xFFFFFFF0);
    
    // Advance tick past rollover: 0x00000020
    g_mock_tick_ms = 0x00000020;
    
    BatteryTelemetry s2;
    ASSERT_TRUE(telemetry_take_sample(&s2));
    
    // s2 must be strictly greater than s1!
    ASSERT_TRUE(s2.timestamp_ms > s1.timestamp_ms);
    uint64_t expected = ((uint64_t)1 << 32) + 0x20;
    ASSERT_TRUE(s2.timestamp_ms == expected);
    
    // Advance normally after rollover
    g_mock_tick_ms = 0x00000050;
    BatteryTelemetry s3;
    ASSERT_TRUE(telemetry_take_sample(&s3));
    ASSERT_TRUE(s3.timestamp_ms == expected + 0x30);
    
    TEST_PASS();
}

void run_hostile_telemetry_tests(void) {
    TEST_SUITE("Hostile Telemetry & Numerical Hardening");
    test_hostile_divide_by_zero_guards();
    test_hostile_telemetry_extreme_values();
    test_monotonic_32bit_tick_rollover();
}
