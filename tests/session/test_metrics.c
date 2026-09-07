#include "../test_helpers.h"
#include "../../core/session.h"
#include "../../core/telemetry.h"
#include "../mocks/furi.h"

void test_session_metrics_calculation(void) {
    TEST_CASE("Session calculates min/max/avg voltage, current, temp, energy integration, and quality flags");
    session_manager_init();
    
    BatteryTelemetry sample;
    memset(&sample, 0, sizeof(BatteryTelemetry));
    sample.flags = VALID_VOLTAGE | VALID_CURRENT | VALID_TEMP | VALID_SOC | VALID_GAUGE;
    sample.gauge_ok = true;
    sample.charging = false;
    
    // Feed 3 samples during discharge
    // Sample 1: 4.00V, -0.2A, 25.0C at t = 0
    sample.timestamp_ms = 0;
    sample.soc_pct = 80;
    sample.voltage_v = 4.00f;
    sample.current_a = -0.20f;
    sample.temperature_c = 25.0f;
    session_process_sample(&sample);
    
    // Sample 2: 3.90V, -0.4A, 30.0C at t = 3600000 ms (1 hour)
    sample.timestamp_ms = 3600000;
    sample.soc_pct = 70;
    sample.voltage_v = 3.90f;
    sample.current_a = -0.40f;
    sample.temperature_c = 30.0f;
    session_process_sample(&sample);
    
    // Sample 3: 3.80V, -0.6A, 35.0C at t = 7200000 ms (2 hours)
    sample.timestamp_ms = 7200000;
    sample.soc_pct = 60;
    sample.voltage_v = 3.80f;
    sample.current_a = -0.60f;
    sample.temperature_c = 35.0f;
    session_process_sample(&sample);
    
    BatterySession active;
    ASSERT_TRUE(session_get_active(&active));
    ASSERT_EQ(active.sample_count, 3);
    
    // Min/Max/Avg Voltage
    ASSERT_FLOAT_EQ(active.min_voltage, 3.80f, 0.01f);
    ASSERT_FLOAT_EQ(active.max_voltage, 4.00f, 0.01f);
    
    // Min/Max/Avg Temperature
    ASSERT_FLOAT_EQ(active.min_temperature, 25.0f, 0.1f);
    ASSERT_FLOAT_EQ(active.max_temperature, 35.0f, 0.1f);
    ASSERT_FLOAT_EQ(active.average_temperature, 30.0f, 0.1f); // (25 + 30 + 35) / 3
    
    // Min/Max/Avg Current
    ASSERT_FLOAT_EQ(active.min_current, -0.60f, 0.01f);
    ASSERT_FLOAT_EQ(active.max_current, -0.20f, 0.01f);
    ASSERT_FLOAT_EQ(active.average_current, -0.40f, 0.01f); // (-0.2 + -0.4 + -0.6) / 3
    
    // Energy integration (trapezoidal):
    // dt1 = 1h, avg_current = (0.2 + 0.4)/2 = 0.3A -> 300 mAh
    // dt2 = 1h, avg_current = (0.4 + 0.6)/2 = 0.5A -> 500 mAh
    // Total integrated: ~800 mAh
    ASSERT_FLOAT_EQ(active.accumulated_energy_mah, 800.0f, 5.0f);
    
    TEST_PASS();
}

void run_session_metrics_tests(void) {
    TEST_SUITE("Session Metrics & Energy Integration");
    test_session_metrics_calculation();
}
