#include "../test_helpers.h"
#include "../../core/telemetry_adapter.h"
#include "../../phase2/charger_hal.h"
#include "../../phase2/charge_policy.h"
#include "../mocks/furi.h"
#include "../mocks/furi_hal_power.h"
#include <string.h>

// ---------------------------------------------------------------------------
// 1. Normalized SI Units & Sanity Checks
// ---------------------------------------------------------------------------
void test_adapter_normalized_units(void) {
    TEST_CASE("Adapter outputs normalized SI units and valid flags");
    telemetry_adapter_init();
    mock_power_reset_defaults();
    g_mock_tick_ms = 5000;

    TelemetryRawSample sample;
    bool ok = telemetry_adapter_read_sample(&sample);

    ASSERT_TRUE(ok);
    ASSERT_EQ(sample.timestamp_ms, 5000);
    ASSERT_FLOAT_EQ(sample.voltage_v, 3.95f, 0.01f);   // Volts
    ASSERT_FLOAT_EQ(sample.current_a, -0.15f, 0.01f);  // Amperes (- = discharge)
    ASSERT_FLOAT_EQ(sample.temperature_c, 28.5f, 0.1f); // Celsius
    ASSERT_EQ(sample.soc_pct, 75);                      // %
    ASSERT_EQ(sample.remaining_capacity_mah, 1575);     // mAh
    ASSERT_EQ(sample.full_capacity_mah, 2050);          // mAh
    ASSERT_EQ(sample.design_capacity_mah, 2100);        // mAh
    ASSERT_EQ(sample.gauge_health_pct, 95);             // %
    ASSERT_TRUE(sample.gauge_ok);
    ASSERT_EQ(sample.state, TelemetryStateNormal);

    // Verify all standard bits set
    uint32_t expected_flags = VALID_VOLTAGE | VALID_CURRENT | VALID_TEMP | VALID_SOC |
                              VALID_REMAINING_CAPACITY | VALID_FULL_CAPACITY |
                              VALID_DESIGN_CAPACITY | VALID_HEALTH | VALID_GAUGE;
    ASSERT_EQ(sample.validity_flags, expected_flags);

    TEST_PASS();
}

// ---------------------------------------------------------------------------
// 2. Hardware Capability Bitmask
// ---------------------------------------------------------------------------
void test_adapter_capability_bitmask(void) {
    TEST_CASE("Platform capabilities reflect full Flipper F7 feature set");
    telemetry_adapter_init();

    uint32_t caps = telemetry_adapter_get_capabilities();
    ASSERT_TRUE(caps & TELEMETRY_CAP_VOLTAGE);
    ASSERT_TRUE(caps & TELEMETRY_CAP_CURRENT);
    ASSERT_TRUE(caps & TELEMETRY_CAP_TEMPERATURE);
    ASSERT_TRUE(caps & TELEMETRY_CAP_SOC);
    ASSERT_TRUE(caps & TELEMETRY_CAP_REMAINING_CAP);
    ASSERT_TRUE(caps & TELEMETRY_CAP_FULL_CAP);
    ASSERT_TRUE(caps & TELEMETRY_CAP_DESIGN_CAP);
    ASSERT_TRUE(caps & TELEMETRY_CAP_HEALTH_PCT);
    ASSERT_TRUE(caps & TELEMETRY_CAP_USB_VOLTAGE);
    ASSERT_TRUE(caps & TELEMETRY_CAP_CHARGING_STATE);
    ASSERT_TRUE(caps & TELEMETRY_CAP_GAUGE_STATUS);

    TEST_PASS();
}

// ---------------------------------------------------------------------------
// 3. Custom Backend Injection (Mock Driver Verification)
// ---------------------------------------------------------------------------
static bool custom_mock_read(TelemetryRawSample* out_sample, void* context) {
    (void)context;
    memset(out_sample, 0, sizeof(TelemetryRawSample));
    out_sample->timestamp_ms = 42000;
    out_sample->voltage_v = 4.12f;
    out_sample->soc_pct = 92;
    out_sample->validity_flags = VALID_VOLTAGE | VALID_SOC;
    out_sample->capabilities = TELEMETRY_CAP_VOLTAGE | TELEMETRY_CAP_SOC;
    out_sample->state = TelemetryStatePartial;
    return true;
}

static uint32_t custom_mock_get_caps(void* context) {
    (void)context;
    return TELEMETRY_CAP_VOLTAGE | TELEMETRY_CAP_SOC;
}

static TelemetryState custom_mock_get_state(void* context) {
    (void)context;
    return TelemetryStatePartial;
}

static const TelemetryAdapterInterface custom_mock_backend = {
    .read = custom_mock_read,
    .get_capabilities = custom_mock_get_caps,
    .get_state = custom_mock_get_state,
};

void test_adapter_mock_backend_injection(void) {
    TEST_CASE("Custom adapter backend injection overrides default platform HAL");
    telemetry_adapter_set_backend(&custom_mock_backend, NULL);

    TelemetryRawSample sample;
    bool ok = telemetry_adapter_read_sample(&sample);
    ASSERT_TRUE(ok);
    ASSERT_EQ(sample.timestamp_ms, 42000);
    ASSERT_FLOAT_EQ(sample.voltage_v, 4.12f, 0.01f);
    ASSERT_EQ(sample.soc_pct, 92);
    ASSERT_EQ(sample.state, TelemetryStatePartial);

    ASSERT_EQ(telemetry_adapter_get_capabilities(), TELEMETRY_CAP_VOLTAGE | TELEMETRY_CAP_SOC);
    ASSERT_EQ(telemetry_adapter_get_state(), TelemetryStatePartial);

    // Reset back to default
    telemetry_adapter_init();
    TEST_PASS();
}

// ---------------------------------------------------------------------------
// 4. Sensor Failure & Operational State Degradation
// ---------------------------------------------------------------------------
void test_adapter_sensor_failure_handling(void) {
    TEST_CASE("Sensor fault / hostile values transition state to Fault or Partial");
    telemetry_adapter_init();
    mock_power_reset_defaults();

    // 1. Gauge failure
    g_mock_power.gauge_ok = false;
    TelemetryRawSample sample;
    telemetry_adapter_read_sample(&sample);
    ASSERT_EQ(sample.state, TelemetryStateFault);
    ASSERT_FALSE(sample.gauge_ok);

    // 2. Hostile / missing voltage
    mock_power_reset_defaults();
    g_mock_power.voltage_v[FuriHalPowerICFuelGauge] = -2.0f;
    telemetry_adapter_read_sample(&sample);
    ASSERT_FALSE(sample.validity_flags & VALID_VOLTAGE);
    ASSERT_EQ(sample.state, TelemetryStatePartial);

    // 3. Extreme hostile temperature (90°C)
    mock_power_reset_defaults();
    g_mock_power.temperature_c[FuriHalPowerICFuelGauge] = 90.0f;
    telemetry_adapter_read_sample(&sample);
    ASSERT_FALSE(sample.validity_flags & VALID_TEMP);
    ASSERT_EQ(sample.state, TelemetryStatePartial);

    TEST_PASS();
}

// ---------------------------------------------------------------------------
// 5. Charger HAL Capabilities Query
// ---------------------------------------------------------------------------
void test_charger_hal_capabilities_query(void) {
    TEST_CASE("Charger HAL capability query reflects active driver features");
    mock_charger_reset();

    uint32_t caps = charger_hal_get_capabilities();
    ASSERT_TRUE(caps & CHARGER_CAP_CHARGE_CONTROL);
    ASSERT_TRUE(caps & CHARGER_CAP_CHARGE_STATUS);
    ASSERT_TRUE(caps & CHARGER_CAP_BATTERY_VOLTAGE);
    ASSERT_TRUE(caps & CHARGER_CAP_BATTERY_TEMP);
    ASSERT_TRUE(caps & CHARGER_CAP_BATTERY_SOC);
    ASSERT_TRUE(caps & CHARGER_CAP_GAUGE_STATUS);

    ASSERT_TRUE(charger_hal_is_available());

    TEST_PASS();
}

// ---------------------------------------------------------------------------
// 6. Passive Production Safe Contract Verification
// ---------------------------------------------------------------------------
// Simulated passive production backend
static bool test_passive_prod_suppress(bool suppress, void* context) {
    (void)suppress;
    (void)context;
    // Strictly fail-closed: refuses hardware write
    return false;
}

static uint32_t test_passive_prod_caps(void* context) {
    (void)context;
    // Note: CHARGER_CAP_CHARGE_CONTROL is omitted!
    return CHARGER_CAP_CHARGE_STATUS | CHARGER_CAP_BATTERY_VOLTAGE | 
           CHARGER_CAP_BATTERY_CURRENT | CHARGER_CAP_BATTERY_TEMP | 
           CHARGER_CAP_BATTERY_SOC | CHARGER_CAP_GAUGE_STATUS;
}

static bool test_passive_prod_available(void* context) {
    (void)context;
    return true;
}

static const ChargerHalInterface passive_prod_contract_backend = {
    .get_soc = NULL,
    .get_voltage = NULL,
    .get_temperature = NULL,
    .is_charging = NULL,
    .is_usb_present = NULL,
    .gauge_ok = NULL,
    .set_charge_suppressed = test_passive_prod_suppress,
    .get_capabilities = test_passive_prod_caps,
    .is_available = test_passive_prod_available,
};

void test_charger_hal_passive_production_contract(void) {
    TEST_CASE("Passive production contract strictly fail-closed (rejects active control)");
    
    // Check capability bits
    uint32_t caps = passive_prod_contract_backend.get_capabilities(NULL);
    ASSERT_FALSE(caps & CHARGER_CAP_CHARGE_CONTROL);
    ASSERT_TRUE(caps & CHARGER_CAP_CHARGE_STATUS);
    ASSERT_TRUE(caps & CHARGER_CAP_BATTERY_VOLTAGE);

    // Verify charge control request returns false (fail-closed safe refusal)
    bool res = passive_prod_contract_backend.set_charge_suppressed(true, NULL);
    ASSERT_FALSE(res);

    res = passive_prod_contract_backend.set_charge_suppressed(false, NULL);
    ASSERT_FALSE(res);

    TEST_PASS();
}

// ---------------------------------------------------------------------------
// 7. Missing Capability Graceful Handling
// ---------------------------------------------------------------------------
void test_charger_hal_missing_capabilities_graceful(void) {
    TEST_CASE("Charger HAL handles missing control capability without fault");
    mock_charger_reset();

    // Strip charge control capability
    mock_charger_set_capabilities(CHARGER_CAP_CHARGE_STATUS | CHARGER_CAP_BATTERY_SOC);

    uint32_t caps = charger_hal_get_capabilities();
    ASSERT_FALSE(caps & CHARGER_CAP_CHARGE_CONTROL);

    // Attempting to suppress charge should fail gracefully
    bool ok = charger_hal_request_charge_disable();
    ASSERT_FALSE(ok);

    mock_charger_reset();
    TEST_PASS();
}

// ---------------------------------------------------------------------------
// 8. Scripted Multi-Phase Deterministic Scenario
// ---------------------------------------------------------------------------
void test_platform_deterministic_scenario(void) {
    TEST_CASE("Deterministic scenario: Boot -> USB -> Chg -> TempExcursion -> Lockout -> Recovery -> Unplug");
    telemetry_adapter_init();
    mock_power_reset_defaults();
    mock_charger_reset();

    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced); // 80% target
    const ChargerHalInterface* hal = charger_hal_get_interface();
    void* ctx = charger_hal_get_context();

    // Phase 1: Boot (Unmanaged, battery 50%, 25°C, no USB)
    mock_charger_set_soc(50);
    mock_charger_set_voltage(3.80f);
    mock_charger_set_temperature(25.0f);
    mock_charger_set_charging(false);
    mock_charger_set_usb_present(false);
    mock_charger_set_gauge_ok(true);

    charge_policy_update(&engine, hal, ctx, 1000);
    ASSERT_EQ(engine.state, ChargeStateUnmanaged);
    ASSERT_FALSE(mock_charger_is_suppressed());

    // Phase 2: USB Plugged In -> Charging Allowed
    mock_charger_set_usb_present(true);
    mock_charger_set_charging(true);
    charge_policy_update(&engine, hal, ctx, 2000);
    ASSERT_EQ(engine.state, ChargeStateChargingAllowed);
    ASSERT_FALSE(mock_charger_is_suppressed());

    // Phase 3: Temperature Excursion (52°C) -> Emergency Thermal Lockout
    mock_charger_set_temperature(52.0f);
    charge_policy_update(&engine, hal, ctx, 3000);
    ASSERT_EQ(engine.state, ChargeStateFault);
    ASSERT_TRUE(mock_charger_is_suppressed()); // Fail-safe suppression engaged

    // Phase 4: Cool Down (28°C) -> Staged Recovery
    mock_charger_set_temperature(28.0f);
    charge_policy_update(&engine, hal, ctx, 4000);
    ASSERT_EQ(engine.state, ChargeStateRecovery); // Two-phase safety recovery

    // Subsequent tick confirms stable recovery -> Charging Allowed
    charge_policy_update(&engine, hal, ctx, 4500);
    ASSERT_EQ(engine.state, ChargeStateChargingAllowed);
    ASSERT_FALSE(mock_charger_is_suppressed());

    // Phase 5: Target Reached (SOC 82% > 80% target)
    mock_charger_set_soc(82);
    charge_policy_update(&engine, hal, ctx, 5000);
    ASSERT_EQ(engine.state, ChargeStateTargetReached);
    ASSERT_FALSE(mock_charger_is_suppressed());

    // Next tick confirms suppression through the HAL.
    charge_policy_update(&engine, hal, ctx, 6000);
    ASSERT_EQ(engine.state, ChargeStateChargeSuppressed);
    ASSERT_TRUE(mock_charger_is_suppressed());

    // Phase 6: USB Disconnect -> Immediate Safe Reset
    mock_charger_set_usb_present(false);
    mock_charger_set_charging(false);
    charge_policy_update(&engine, hal, ctx, 7000);
    ASSERT_EQ(engine.state, ChargeStateUnmanaged);
    ASSERT_FALSE(mock_charger_is_suppressed());

    TEST_PASS();
}

// ---------------------------------------------------------------------------
// Suite Runner
// ---------------------------------------------------------------------------
void run_platform_adapter_tests(void) {
    TEST_SUITE("Platform Adapter & Hardware Abstraction");
    test_adapter_normalized_units();
    test_adapter_capability_bitmask();
    test_adapter_mock_backend_injection();
    test_adapter_sensor_failure_handling();
    test_charger_hal_capabilities_query();
    test_charger_hal_passive_production_contract();
    test_charger_hal_missing_capabilities_graceful();
    test_platform_deterministic_scenario();
}
