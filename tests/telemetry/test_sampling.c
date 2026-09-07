#include "../test_helpers.h"
#include "../../core/telemetry.h"
#include "../mocks/furi.h"
#include "../mocks/furi_hal_power.h"

void test_sampling_intervals(void) {
    TEST_CASE("Adaptive sampling intervals match specification");
    
    telemetry_set_sampling_mode(SamplingModeIdle);
    ASSERT_EQ(telemetry_get_interval_ms(), 30000);
    
    telemetry_set_sampling_mode(SamplingModeActive);
    ASSERT_EQ(telemetry_get_interval_ms(), 10000);
    
    telemetry_set_sampling_mode(SamplingModeCharging);
    ASSERT_EQ(telemetry_get_interval_ms(), 5000);
    
    telemetry_set_sampling_mode(SamplingModeDischarging);
    ASSERT_EQ(telemetry_get_interval_ms(), 10000);
    
    telemetry_set_sampling_mode(SamplingModeAnomaly);
    ASSERT_EQ(telemetry_get_interval_ms(), 1000);
    
    TEST_PASS();
}

void test_sampling_automatic_state_transitions(void) {
    TEST_CASE("State transitions adaptively switch sampling intervals");
    mock_power_reset_defaults();
    
    // Default idle
    g_mock_power.is_charging = false;
    g_mock_power.current_a[FuriHalPowerICFuelGauge] = -0.01f;
    BatteryTelemetry sample;
    telemetry_take_sample(&sample);
    ASSERT_EQ(telemetry_get_interval_ms(), 30000);
    
    // Switch to charging
    g_mock_power.is_charging = true;
    telemetry_take_sample(&sample);
    ASSERT_EQ(telemetry_get_interval_ms(), 5000);
    
    // Switch to active discharge
    g_mock_power.is_charging = false;
    g_mock_power.current_a[FuriHalPowerICFuelGauge] = -0.35f;
    telemetry_take_sample(&sample);
    ASSERT_EQ(telemetry_get_interval_ms(), 10000);
    
    // Back to idle
    g_mock_power.current_a[FuriHalPowerICFuelGauge] = 0.0f;
    telemetry_take_sample(&sample);
    ASSERT_EQ(telemetry_get_interval_ms(), 30000);
    
    TEST_PASS();
}

void run_telemetry_sampling_tests(void) {
    TEST_SUITE("Telemetry Sampling");
    test_sampling_intervals();
    test_sampling_automatic_state_transitions();
}
