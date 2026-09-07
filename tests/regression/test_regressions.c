#include "../test_helpers.h"
#include "../../core/session.h"
#include "../../storage/journal.h"
#include "../../phase2/battery_health.h"
#include "../../phase2/charge_policy.h"
#include "../../phase2/charger_hal.h"
#include <storage/storage.h>
#include <string.h>
#include <math.h>

// REG_01: In session.c, newly allocated sessions must initialize SESSION_QUALITY_NO_TEMP_EXCURSION
void test_reg_01_session_quality_flags_init(void) {
    TEST_CASE("REG_01: session quality flags initialize NO_TEMP_EXCURSION");
    session_manager_init();
    
    BatteryTelemetry sample;
    memset(&sample, 0, sizeof(BatteryTelemetry));
    sample.timestamp_ms = 1000;
    sample.voltage_v = 4.10f;
    sample.current_a = 0.5f; // Charging current
    sample.temperature_c = 25.0f; // Normal room temp
    sample.soc_pct = 50;
    sample.charging = true;
    sample.flags = VALID_VOLTAGE | VALID_CURRENT | VALID_TEMP | VALID_SOC;
    
    session_process_sample(&sample);
    
    BatterySession active;
    ASSERT_TRUE(session_get_active(&active));
    ASSERT_EQ(active.type, SessionTypeCharging);
    
    // Explicitly verify NO_TEMP_EXCURSION was initialized and not zeroed out
    ASSERT_TRUE((active.quality_flags & SESSION_QUALITY_NO_TEMP_EXCURSION) != 0);
    
    TEST_PASS();
}

// REG_02: Journal recovery must not cast offset to uint16_t (>64KB file truncation bug)
void test_reg_02_journal_16bit_cast_truncation(void) {
    TEST_CASE("REG_02: journal recovery supports 64-bit offsets without truncation");
    mock_storage_reset();
    const char* path = "/ext/reg_journal.dat";
    
    ASSERT_TRUE(journal_init(&g_mock_storage, path));
    
    // Write 70 KB of valid records
    BatteryTelemetry sample;
    memset(&sample, 0, sizeof(BatteryTelemetry));
    sample.voltage_v = 3.85f;
    sample.soc_pct = 80;
    sample.flags = VALID_VOLTAGE | VALID_SOC;
    
    for(uint32_t i = 0; i < 900; i++) {
        sample.timestamp_ms = 10000 + i * 100;
        journal_write_record(RecordTypeSample, 1, sample.timestamp_ms, &sample, sizeof(BatteryTelemetry));
    }
    
    journal_free();
    
    File* f = storage_file_alloc(&g_mock_storage);
    ASSERT_TRUE(storage_file_open(f, path, FSAM_READ, FSOM_OPEN_EXISTING));
    uint64_t full_size = storage_file_size(f);
    ASSERT_TRUE(full_size > 65536);
    storage_file_close(f);
    storage_file_free(f);
    
    // Recovery of intact file must preserve all >64KB bytes
    uint32_t rec_count = 0;
    uint64_t rec_bytes = 0;
    ASSERT_TRUE(journal_recover(&g_mock_storage, path, &rec_count, &rec_bytes));
    ASSERT_TRUE(rec_bytes == full_size);
    ASSERT_EQ(rec_count, 900);
    
    TEST_PASS();
}

// REG_03: Reference capacity of 0.0f must not produce NaN in observed health
void test_reg_03_zero_reference_capacity_nan(void) {
    TEST_CASE("REG_03: reference capacity of 0.0f must not produce NaN");
    BatteryHealthEngine* engine = battery_health_alloc();
    engine->has_estimate = true;
    engine->estimator.robust_estimate_mah = 2050.0f;
    engine->reference_capacity_mah = 0.0f; // Potential divide-by-zero
    
    BatteryHealthSnapshot snap;
    battery_health_get_snapshot(engine, 5000, &snap);
    
    ASSERT_EQ(snap.observed_health_pct, 0);
    ASSERT_TRUE(!isnan((double)snap.observed_health_pct));
    
    battery_health_free(engine);
    TEST_PASS();
}

// REG_04: Storage handle lifetime safety verification
void test_reg_04_storage_lifetime_safety(void) {
    TEST_CASE("REG_04: RECORD_STORAGE handle held for full application lifetime");
    mock_storage_reset();
    Storage* st = furi_record_open(RECORD_STORAGE);
    ASSERT_TRUE(st != NULL);
    
    // Simulating app lifecycle: st is held across active usage
    bool init_ok = journal_init(st, "/ext/lifetime_test.dat");
    ASSERT_TRUE(init_ok);
    
    // Write sample
    BatteryTelemetry s = { .timestamp_ms = 1000, .voltage_v = 4.0f, .flags = VALID_VOLTAGE };
    ASSERT_TRUE(journal_write_record(RecordTypeSample, 1, 1000, &s, sizeof(BatteryTelemetry)));
    
    // Cleanup in correct order
    journal_free();
    furi_record_close(RECORD_STORAGE);
    
    TEST_PASS();
}

// REG_05: USB disconnect must force safe unmanaged charging state
void test_reg_05_charge_policy_usb_disconnect_fail_safe(void) {
    TEST_CASE("REG_05: USB disconnect forces unmanaged & unsuppressed fail-safe");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced); // 80% target
    
    const ChargerHalInterface* hal = charger_hal_get_interface();
    void* ctx = charger_hal_get_context();
    
    // State 1: Charging below target
    mock_charger_set_soc(75);
    mock_charger_set_usb_present(true);
    mock_charger_set_charging(true);
    charge_policy_update(&engine, hal, ctx, 1000);
    ASSERT_EQ(engine.state, ChargeStateChargingAllowed);
    
    // State 2: Target reached -> charge suppressed
    mock_charger_set_soc(81);
    charge_policy_update(&engine, hal, ctx, 2000);
    ASSERT_EQ(engine.state, ChargeStateTargetReached);
    ASSERT_TRUE(mock_charger_is_suppressed());
    
    // State 3: USB disconnected -> MUST IMMEDIATELY TRANSITION TO UNMANAGED & UNSUPPRESSED
    mock_charger_set_usb_present(false);
    charge_policy_update(&engine, hal, ctx, 3000);
    ASSERT_EQ(engine.state, ChargeStateUnmanaged);
    ASSERT_FALSE(mock_charger_is_suppressed());
    
    TEST_PASS();
}

void run_regression_tests(void) {
    TEST_SUITE("Permanent Regression Suite (REG_01 - REG_05)");
    test_reg_01_session_quality_flags_init();
    test_reg_02_journal_16bit_cast_truncation();
    test_reg_03_zero_reference_capacity_nan();
    test_reg_04_storage_lifetime_safety();
    test_reg_05_charge_policy_usb_disconnect_fail_safe();
}
