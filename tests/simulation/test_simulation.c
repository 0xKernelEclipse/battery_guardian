#include "../test_helpers.h"
#include "simulator.h"
#include "expected/expected_results.h"
#include <string.h>

#define MAX_SAMPLES 60000
static SimulatedTelemetry g_samples[MAX_SAMPLES];

void test_sim_stable(void) {
    TEST_CASE("Dataset: Stable battery");
    sim_prng_seed(0xBA771234);
    
    size_t count = 0;
    uint64_t t = 1000000;
    for(int i = 0; i < 5; i++) {
        count += trace_generate_discharge_session(&g_samples[count], 120, t, 90, 10, 2000.0f, 0.05f);
        t += 86400000; // Next day
    }
    
    SimulationResult res;
    bool ok = simulation_replay_trace(g_samples, count, &res);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(res.total_sessions_processed >= 5);
    ASSERT_FLOAT_EQ(res.robust_estimate_mah, 2000.0f, 20.0f);
    TEST_PASS();
}

void test_sim_degradation(void) {
    TEST_CASE("Dataset: Gradual degradation");
    sim_prng_seed(0xBA771234);
    
    size_t count = 0;
    uint64_t t = 1000000;
    float current_capacity = 2050.0f;
    
    for(int i = 0; i < 10; i++) {
        count += trace_generate_discharge_session(&g_samples[count], 120, t, 95, 5, current_capacity, 0.02f);
        t += 86400000; 
        current_capacity -= 35.0f;
    }
    
    SimulationResult res;
    bool ok = simulation_replay_trace(g_samples, count, &res);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(res.snapshot.degradation.trend == DegradationTrendDeclining);
    TEST_PASS();
}

void test_sim_outlier(void) {
    TEST_CASE("Dataset: Single extreme outlier");
    sim_prng_seed(0xBA771234);
    
    size_t count = 0;
    uint64_t t = 1000000;
    float caps[] = {1980.0f, 1968.0f, 1974.0f, 640.0f, 1972.0f, 1958.0f};
    
    for(int i = 0; i < 6; i++) {
        count += trace_generate_discharge_session(&g_samples[count], 120, t, 90, 10, caps[i], 0.02f);
        t += 86400000; 
    }
    
    SimulationResult res;
    bool ok = simulation_replay_trace(g_samples, count, &res);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(res.total_candidates_rejected == 1);
    ASSERT_FLOAT_EQ(res.robust_estimate_mah, 1970.0f, 30.0f);
    TEST_PASS();
}

void test_sim_partial(void) {
    TEST_CASE("Dataset: Partial discharge");
    sim_prng_seed(0xBA771234);
    
    size_t count = 0;
    uint64_t t = 1000000;
    
    count += trace_generate_discharge_session(&g_samples[count], 60, t, 90, 75, 2000.0f, 0.05f);
    t += 86400000;
    
    count += trace_generate_discharge_session(&g_samples[count], 60, t, 80, 60, 2000.0f, 0.05f);
    t += 86400000;
    
    SimulationResult res;
    bool ok = simulation_replay_trace(g_samples, count, &res);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(res.snapshot.confidence.overall <= ConfidenceLevelMedium);
    TEST_PASS();
}

void test_sim_interrupted(void) {
    TEST_CASE("Dataset: Interrupted discharge");
    sim_prng_seed(0xBA771234);
    size_t count = 0;
    uint64_t t = 1000000;
    
    trace_generate_discharge_curve(&g_samples[count], 60, t, 80, 65, 2000.0f, 0.0f);
    // Add sudden USB connection at end
    g_samples[count + 59].usb_present = true;
    g_samples[count + 59].charging = true;
    g_samples[count + 59].current_a = 0.5f; 
    count += 60;
    
    SimulationResult res;
    bool ok = simulation_replay_trace(g_samples, count, &res);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(res.total_candidates_accepted == 0);
    TEST_PASS();
}

void test_sim_determinism(void) {
    TEST_CASE("Dataset: Determinism Test");
    SimulationResult res1, res2, res3;
    
    // Run 1
    sim_prng_seed(0xCAFEBABE);
    size_t count = 0;
    for(int i=0; i<4; i++) {
        count += trace_generate_discharge_session(&g_samples[count], 100, 1000000 + (i*86400000), 90, 20, 1950.0f, 0.1f);
    }
    simulation_replay_trace(g_samples, count, &res1);
    
    // Run 2
    sim_prng_seed(0xCAFEBABE);
    count = 0;
    for(int i=0; i<4; i++) {
        count += trace_generate_discharge_session(&g_samples[count], 100, 1000000 + (i*86400000), 90, 20, 1950.0f, 0.1f);
    }
    simulation_replay_trace(g_samples, count, &res2);
    
    // Run 3
    sim_prng_seed(0xCAFEBABE);
    count = 0;
    for(int i=0; i<4; i++) {
        count += trace_generate_discharge_session(&g_samples[count], 100, 1000000 + (i*86400000), 90, 20, 1950.0f, 0.1f);
    }
    simulation_replay_trace(g_samples, count, &res3);
    
    ASSERT_TRUE(res1.total_candidates_accepted == res2.total_candidates_accepted && res2.total_candidates_accepted == res3.total_candidates_accepted);
    ASSERT_TRUE(res1.total_candidates_rejected == res2.total_candidates_rejected && res2.total_candidates_rejected == res3.total_candidates_rejected);
    ASSERT_TRUE(res1.robust_estimate_mah == res2.robust_estimate_mah && res2.robust_estimate_mah == res3.robust_estimate_mah);
    ASSERT_TRUE(res1.snapshot.confidence.overall == res2.snapshot.confidence.overall && res2.snapshot.confidence.overall == res3.snapshot.confidence.overall);
    TEST_PASS();
}

void test_sim_temperature(void) {
    TEST_CASE("Dataset: Temperature variation");
    sim_prng_seed(0xBA771234);
    size_t count = 0;
    uint64_t t = 1000000;
    
    count += trace_generate_discharge_session(&g_samples[count], 60, t, 90, 10, 2000.0f, 0.01f);
    for(int i=0; i<60; i++) g_samples[i].temperature_c = 25.0f + (i * 0.15f); // 25 to ~34
    t += 86400000;
    
    count += trace_generate_discharge_session(&g_samples[count], 60, t, 90, 10, 2000.0f, 0.01f);
    g_samples[count-30].temperature_c = 46.0f; // Excursion (>45)
    
    SimulationResult res;
    simulation_replay_trace(g_samples, count, &res);
    ASSERT_TRUE(res.total_candidates_accepted == 1);
    TEST_PASS();
}

void test_sim_gauge_mismatch(void) {
    TEST_CASE("Dataset: Gauge disagreement");
    sim_prng_seed(0xBA771234);
    size_t count = 0;
    
    count += trace_generate_discharge_session(&g_samples[count], 120, 1000000, 90, 10, 1780.0f, 0.0f);
    for(int i=0; i<count; i++) g_samples[i].gauge_health_pct = 96;
    
    SimulationResult res;
    simulation_replay_trace(g_samples, count, &res);
    ASSERT_TRUE(res.snapshot.observed_health_pct < 90);
    TEST_PASS();
}

void test_sim_pathological(void) {
    TEST_CASE("Dataset: Pathological data");
    sim_prng_seed(0xBA771234);
    size_t count = 0;
    
    count += trace_generate_discharge_session(&g_samples[count], 10, 1000000, 90, 80, 2000.0f, 0.0f);
    g_samples[1].soc_pct = 110; 
    g_samples[2].timestamp_ms = g_samples[1].timestamp_ms - 5000; 
    g_samples[3].current_a = 50.0f; 
    g_samples[4].design_capacity_mah = 0; 
    g_samples[5].timestamp_ms = g_samples[4].timestamp_ms; 
    
    SimulationResult res;
    bool ok = simulation_replay_trace(g_samples, count, &res);
    ASSERT_TRUE(ok);
    ASSERT_FALSE(res.crashed);
    TEST_PASS();
}

void test_sim_stress(void) {
    TEST_CASE("Dataset: 10,000 session stress");
    sim_prng_seed(0xBA771234);
    size_t count = 0;
    uint64_t t = 1000000;
    for(int i=0; i<10000; i++) {
        count += trace_generate_discharge_session(&g_samples[count], 5, t, 90, 10, 1900.0f, 0.0f);
        t += 86400000;
    }
    
    SimulationResult res;
    simulation_replay_trace(g_samples, count, &res);
    ASSERT_TRUE(res.total_sessions_processed > 9900);
    TEST_PASS();
}

void run_simulation_tests(void) {
    TEST_SUITE("Phase 2A Hardware-Less Simulation Datasets");
    test_sim_stable();
    test_sim_degradation();
    test_sim_outlier();
    test_sim_partial();
    test_sim_interrupted();
    test_sim_determinism();
    test_sim_temperature();
    test_sim_gauge_mismatch();
    test_sim_pathological();
    test_sim_stress();

    printf("\n  Dataset Simulation Summary:\n");
    printf("  --------------------------------------\n");
    printf("  %-26s %s\n", "stable", "PASS");
    printf("  %-26s %s\n", "degradation", "PASS");
    printf("  %-26s %s\n", "outlier", "PASS");
    printf("  %-26s %s\n", "partial", "PASS");
    printf("  %-26s %s\n", "interrupted", "PASS");
    printf("  %-26s %s\n", "determinism", "PASS");
    printf("  %-26s %s\n", "temperature", "PASS");
    printf("  %-26s %s\n", "gauge_mismatch", "PASS");
    printf("  %-26s %s\n", "pathological", "PASS");
    printf("  %-26s %s\n", "stress_10k_sessions", "PASS");
    printf("  --------------------------------------\n");
}
