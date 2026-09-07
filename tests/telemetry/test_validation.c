#include "../test_helpers.h"
#include "../../core/telemetry.h"
#include "../mocks/furi.h"
#include "../mocks/furi_hal_power.h"

void test_telemetry_valid_sample(void) {
    TEST_CASE("Valid sample accepted with full bitmask");
    mock_power_reset_defaults();
    g_mock_tick_ms = 1000;
    
    BatteryTelemetry sample;
    bool ok = telemetry_take_sample(&sample);
    
    ASSERT_TRUE(ok);
    ASSERT_EQ(sample.timestamp_ms, 1000);
    ASSERT_FLOAT_EQ(sample.voltage_v, 3.95f, 0.01f);
    ASSERT_FLOAT_EQ(sample.current_a, -0.15f, 0.01f);
    ASSERT_FLOAT_EQ(sample.temperature_c, 28.5f, 0.1f);
    ASSERT_EQ(sample.soc_pct, 75);
    ASSERT_EQ(sample.remaining_capacity_mah, 1575);
    ASSERT_EQ(sample.full_capacity_mah, 2050);
    ASSERT_EQ(sample.design_capacity_mah, 2100);
    ASSERT_EQ(sample.gauge_health_pct, 95);
    ASSERT_TRUE(sample.gauge_ok);
    
    // Check bitmask
    uint32_t expected = VALID_VOLTAGE | VALID_CURRENT | VALID_TEMP | VALID_SOC |
                        VALID_REMAINING_CAPACITY | VALID_FULL_CAPACITY |
                        VALID_DESIGN_CAPACITY | VALID_HEALTH | VALID_GAUGE;
    ASSERT_EQ(sample.flags, expected);
    
    TEST_PASS();
}

void test_telemetry_invalid_voltage(void) {
    TEST_CASE("Invalid voltage sets validity bit without fabricating value");
    mock_power_reset_defaults();
    g_mock_power.voltage_v[FuriHalPowerICFuelGauge] = -1.0f; // Invalid
    
    BatteryTelemetry sample;
    telemetry_take_sample(&sample);
    
    ASSERT_FALSE(sample.flags & VALID_VOLTAGE);
    ASSERT_FLOAT_EQ(sample.voltage_v, -1.0f, 0.01f); // Raw value preserved, not fabricated
    ASSERT_TRUE(sample.flags & VALID_CURRENT);        // Rest of sample intact
    ASSERT_TRUE(sample.flags & VALID_TEMP);
    
    // Test over-voltage
    g_mock_power.voltage_v[FuriHalPowerICFuelGauge] = 6.5f;
    telemetry_take_sample(&sample);
    ASSERT_FALSE(sample.flags & VALID_VOLTAGE);
    
    TEST_PASS();
}

void test_telemetry_invalid_temperature(void) {
    TEST_CASE("Invalid temperature sets validity bit correctly");
    mock_power_reset_defaults();
    g_mock_power.temperature_c[FuriHalPowerICFuelGauge] = -30.0f; // Below physical limits
    
    BatteryTelemetry sample;
    telemetry_take_sample(&sample);
    ASSERT_FALSE(sample.flags & VALID_TEMP);
    ASSERT_FLOAT_EQ(sample.temperature_c, -30.0f, 0.01f);
    ASSERT_TRUE(sample.flags & VALID_VOLTAGE);
    
    g_mock_power.temperature_c[FuriHalPowerICFuelGauge] = 85.0f; // Above physical limits
    telemetry_take_sample(&sample);
    ASSERT_FALSE(sample.flags & VALID_TEMP);
    
    TEST_PASS();
}

void test_telemetry_invalid_soc(void) {
    TEST_CASE("SOC > 100% is flagged as invalid");
    mock_power_reset_defaults();
    g_mock_power.soc_pct = 150; // Out of bounds
    
    BatteryTelemetry sample;
    telemetry_take_sample(&sample);
    ASSERT_FALSE(sample.flags & VALID_SOC);
    ASSERT_EQ(sample.soc_pct, 150);
    ASSERT_TRUE(sample.flags & VALID_VOLTAGE);
    
    TEST_PASS();
}

void test_telemetry_invalid_capacities_and_gauge(void) {
    TEST_CASE("Zero/invalid capacity and gauge failure handled gracefully");
    mock_power_reset_defaults();
    g_mock_power.remaining_capacity_mah = 0;
    g_mock_power.full_capacity_mah = 0;
    g_mock_power.design_capacity_mah = 0;
    g_mock_power.health_pct = 200; // invalid health
    g_mock_power.gauge_ok = false;
    
    BatteryTelemetry sample;
    telemetry_take_sample(&sample);
    
    ASSERT_FALSE(sample.flags & VALID_REMAINING_CAPACITY);
    ASSERT_FALSE(sample.flags & VALID_FULL_CAPACITY);
    ASSERT_FALSE(sample.flags & VALID_DESIGN_CAPACITY);
    ASSERT_FALSE(sample.flags & VALID_HEALTH);
    ASSERT_FALSE(sample.gauge_ok);
    ASSERT_TRUE(sample.flags & VALID_GAUGE); // Valid gauge query performed, result was gauge not ok
    
    TEST_PASS();
}

void run_telemetry_validation_tests(void) {
    TEST_SUITE("Telemetry Validation");
    test_telemetry_valid_sample();
    test_telemetry_invalid_voltage();
    test_telemetry_invalid_temperature();
    test_telemetry_invalid_soc();
    test_telemetry_invalid_capacities_and_gauge();
}
