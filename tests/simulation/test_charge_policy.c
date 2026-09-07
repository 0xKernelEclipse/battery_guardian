#include "../test_helpers.h"
#include "../../phase2/charge_policy.h"
#include "../../phase2/charger_hal.h"
#include "trace.h"
#include <stdio.h>
#include <string.h>

static void print_trace_row(uint64_t t, uint8_t soc, const char* state, const char* action) {
    // Standard command trace output
    printf("%05lu   %2d%%    %-18s %s\n", (unsigned long)(t / 1000), soc, state, action);
}

void test_policy_full(void) {
    TEST_CASE("Policy: FULL policy logic");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyFull);
    
    mock_charger_set_soc(95);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateChargingAllowed);
    ASSERT_FALSE(mock_charger_is_suppressed());
    TEST_PASS();
}

void test_policy_balanced(void) {
    TEST_CASE("Policy: BALANCED policy logic");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_soc(79);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateChargingAllowed);
    
    mock_charger_set_soc(80);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 2000);
    ASSERT_TRUE(engine.state == ChargeStateTargetReached);
    TEST_PASS();
}

void test_policy_lifespan(void) {
    TEST_CASE("Policy: LIFESPAN policy logic");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyLifespan);
    
    mock_charger_set_soc(59);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateChargingAllowed);
    
    mock_charger_set_soc(60);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 2000);
    ASSERT_TRUE(engine.state == ChargeStateTargetReached);
    TEST_PASS();
}

void test_policy_custom(void) {
    TEST_CASE("Policy: CUSTOM policy logic");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyCustom);
    charge_policy_set_custom(&engine, 70, 5);
    
    mock_charger_set_soc(69);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateChargingAllowed);
    
    mock_charger_set_soc(70);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 2000);
    ASSERT_TRUE(engine.state == ChargeStateTargetReached);
    TEST_PASS();
}

void test_policy_hysteresis_oscillation(void) {
    TEST_CASE("Policy: hysteresis prevention");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced); // 80% target, 3% hysteresis
    
    mock_charger_set_soc(80);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 2000);
    ASSERT_TRUE(engine.state == ChargeStateChargeSuppressed);
    
    // SOC oscillates slightly down: 80 -> 79 -> 80 -> 79 -> 80 -> 79
    uint64_t t = 3000;
    for (int i = 0; i < 5; i++) {
        mock_charger_set_soc(79);
        charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), t);
        ASSERT_TRUE(engine.state == ChargeStateChargeSuppressed);
        t += 1000;
        
        mock_charger_set_soc(80);
        charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), t);
        ASSERT_TRUE(engine.state == ChargeStateChargeSuppressed);
        t += 1000;
    }
    TEST_PASS();
}

void test_policy_target_reached(void) {
    TEST_CASE("Policy: target reached transition");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_soc(80);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateTargetReached);
    TEST_PASS();
}

void test_policy_target_recovery(void) {
    TEST_CASE("Policy: target recovery transition");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_soc(80);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 2000);
    ASSERT_TRUE(engine.state == ChargeStateChargeSuppressed);
    
    // Drops past hysteresis (80 - 3 = 77)
    mock_charger_set_soc(76);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 3000);
    ASSERT_TRUE(engine.state == ChargeStateChargingAllowed);
    TEST_PASS();
}

void test_policy_usb_disconnect(void) {
    TEST_CASE("Policy: USB disconnect safe state");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_soc(70);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateChargingAllowed);
    
    mock_charger_set_usb_present(false);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 2000);
    ASSERT_TRUE(engine.state == ChargeStateUnmanaged);
    ASSERT_FALSE(mock_charger_is_suppressed());
    TEST_PASS();
}

void test_policy_invalid_soc(void) {
    TEST_CASE("Policy: invalid SOC safety lockout");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_soc(110); // Invalid SOC
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateFault);
    ASSERT_TRUE(mock_charger_is_suppressed());
    TEST_PASS();
}

void test_policy_invalid_voltage(void) {
    TEST_CASE("Policy: invalid voltage safety lockout");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_voltage(4.6f); // Invalid high voltage
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateFault);
    ASSERT_TRUE(mock_charger_is_suppressed());
    TEST_PASS();
}

void test_policy_invalid_temperature(void) {
    TEST_CASE("Policy: invalid temperature safety lockout");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_temperature(46.0f); // Hot spike
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateFault);
    ASSERT_TRUE(mock_charger_is_suppressed());
    TEST_PASS();
}

void test_policy_gauge_failure(void) {
    TEST_CASE("Policy: gauge failure lockout");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_gauge_ok(false);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateFault);
    ASSERT_TRUE(mock_charger_is_suppressed());
    TEST_PASS();
}

void test_policy_charger_rejection(void) {
    TEST_CASE("Policy: charger command rejection");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_soc(80);
    mock_charger_set_cmd_rejection(true);
    
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateTargetReached);
    TEST_PASS();
}

void test_policy_charger_disappearance(void) {
    TEST_CASE("Policy: charger disappearance");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_disappeared(true);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateFault);
    TEST_PASS();
}

void test_policy_controller_restart(void) {
    TEST_CASE("Policy: controller restart deterministic state");
    mock_charger_reset();
    
    // Simulate restart by allocating a fresh engine struct while battery is at 85% with an 80% policy limit.
    // It must boot into UNMANAGED, and the first update must resolve it safely.
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    ASSERT_TRUE(engine.state == ChargeStateUnmanaged);
    
    mock_charger_set_soc(85);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateTargetReached);
    TEST_PASS();
}

void test_policy_change_during_charge(void) {
    TEST_CASE("Policy: policy change during charge");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced); // target 80%
    
    mock_charger_set_soc(70);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateChargingAllowed);
    
    // Change policy to LIFESPAN (target 60%)
    charge_policy_set_type(&engine, ChargePolicyLifespan);
    ASSERT_TRUE(engine.state == ChargeStateUnmanaged);
    
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 2000);
    ASSERT_TRUE(engine.state == ChargeStateTargetReached);
    TEST_PASS();
}

void test_policy_fault_recovery(void) {
    TEST_CASE("Policy: fault recovery validation");
    mock_charger_reset();
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    mock_charger_set_temperature(48.0f); // fault
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 1000);
    ASSERT_TRUE(engine.state == ChargeStateFault);
    
    // Clear fault
    mock_charger_set_temperature(25.0f);
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 2000);
    // Fault -> Recovery
    ASSERT_TRUE(engine.state == ChargeStateRecovery);
    
    charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), 3000);
    ASSERT_TRUE(engine.state == ChargeStateChargingAllowed);
    TEST_PASS();
}

void test_policy_deterministic_replay(void) {
    TEST_CASE("Policy: deterministic replay consistency");
    
    ChargePolicyEngine eng1, eng2;
    charge_policy_init(&eng1, ChargePolicyBalanced);
    charge_policy_init(&eng2, ChargePolicyBalanced);
    
    // Seed PRNG
    sim_prng_seed(0xBA77C0DE);
    
    // Generate identical inputs and update both engines
    for(int i = 0; i < 50; i++) {
        uint8_t soc = (uint8_t)sim_prng_range(60, 90);
        float voltage = sim_prng_range(3.5f, 4.2f);
        float temp = sim_prng_range(20.0f, 40.0f);
        
        mock_charger_set_soc(soc);
        mock_charger_set_voltage(voltage);
        mock_charger_set_temperature(temp);
        
        charge_policy_update(&eng1, charger_hal_get_interface(), charger_hal_get_context(), i * 1000);
    }
    
    // Reset and replay identical sequence on eng2
    mock_charger_reset();
    sim_prng_seed(0xBA77C0DE);
    for(int i = 0; i < 50; i++) {
        uint8_t soc = (uint8_t)sim_prng_range(60, 90);
        float voltage = sim_prng_range(3.5f, 4.2f);
        float temp = sim_prng_range(20.0f, 40.0f);
        
        mock_charger_set_soc(soc);
        mock_charger_set_voltage(voltage);
        mock_charger_set_temperature(temp);
        
        charge_policy_update(&eng2, charger_hal_get_interface(), charger_hal_get_context(), i * 1000);
    }
    
    ASSERT_TRUE(eng1.state == eng2.state);
    ASSERT_TRUE(eng1.transition_count == eng2.transition_count);
    TEST_PASS();
}

void test_policy_bounded_memory(void) {
    TEST_CASE("Policy: bounded memory");
    
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    size_t size = sizeof(engine);
    ASSERT_TRUE(size < 256);
    TEST_PASS();
}

void test_policy_fuzzing_100k(void) {
    TEST_CASE("Policy: 100,000 transition property stress");
    mock_charger_reset();
    sim_prng_seed(0xBA779999);
    
    ChargePolicyEngine engine;
    charge_policy_init(&engine, ChargePolicyBalanced);
    
    uint64_t current_time_ms = 1000000;
    
    // Sample tracing command table (first 5 updates)
    printf("\n    Sample Command Trace:\n");
    printf("    TIME    SOC    STATE             ACTION\n");
    printf("    ------------------------------------------------\n");
    
    uint32_t violations = 0;
    
    for (int i = 0; i < 100000; i++) {
        current_time_ms += 10000; // 10s steps
        
        // Setup inputs
        bool usb = (sim_prng_range(0, 100) < 95);
        bool gauge = (sim_prng_range(0, 100) < 98);
        uint8_t soc = 50;
        if (sim_prng_range(0, 100) < 5) {
            soc = (uint8_t)sim_prng_range(0, 110);
        } else {
            soc = (uint8_t)sim_prng_range(50, 90);
        }
        float voltage = 3.2f + (soc / 100.0f) * 0.9f + sim_prng_range(-0.1f, 0.1f);
        if (sim_prng_range(0, 100) < 1) {
            voltage = sim_prng_range(1.0f, 6.0f);
        }
        float temp = 25.0f;
        if (sim_prng_range(0, 100) < 2) {
            temp = sim_prng_range(-10.0f, 65.0f);
        } else {
            temp += sim_prng_range(-1.0f, 1.0f);
        }
        
        // Apply mock inputs
        mock_charger_set_soc(soc);
        mock_charger_set_voltage(voltage);
        mock_charger_set_temperature(temp);
        mock_charger_set_usb_present(usb);
        mock_charger_set_gauge_ok(gauge);
        
        // Random policy changes
        if (sim_prng_range(0, 100) < 1) {
            int p = (int)sim_prng_range(0, 3);
            if (p == 0) charge_policy_set_type(&engine, ChargePolicyLifespan);
            else if (p == 1) charge_policy_set_type(&engine, ChargePolicyBalanced);
            else charge_policy_set_type(&engine, ChargePolicyFull);
        }
        
        charge_policy_update(&engine, charger_hal_get_interface(), charger_hal_get_context(), current_time_ms);
        
        // Optionally print first few iterations for trace verification
        if (i < 5) {
            printf("    ");
            print_trace_row(current_time_ms, soc, charge_policy_state_name(engine.state), 
                            mock_charger_is_suppressed() ? "SUPPRESS" : "ALLOW");
            if (i == 4) {
                printf("    ------------------------------------------------\n");
            }
        }
        
        // Invariant verification
        const char* violation_reason = NULL;
        
        if (!(engine.state >= ChargeStateUnmanaged && engine.state <= ChargeStateRecovery)) {
            violation_reason = "Invalid engine state";
        } else if (!(engine.policy.target_soc >= 50 && engine.policy.target_soc <= 100)) {
            violation_reason = "Target SOC out of bounds";
        } else if (engine.state == ChargeStateChargingAllowed && mock_charger_is_suppressed() == false && usb) {
            if (soc >= (uint8_t)engine.policy.target_soc) {
                violation_reason = "Charge enabled above limit";
            } else if (temp > 45.0f) {
                violation_reason = "Charge enabled during high temp excursion";
            } else if (temp < 0.0f) {
                violation_reason = "Charge enabled during subzero temp";
            } else if (!gauge) {
                violation_reason = "Charge enabled during gauge fault";
            } else if (voltage < 3.0f || voltage > 4.5f) {
                violation_reason = "Charge enabled during voltage fault";
            } else if (soc > 100) {
                violation_reason = "Charge enabled with invalid SOC";
            }
        } else if (mock_charger_is_suppressed() && usb) {
            if (!(engine.state == ChargeStateTargetReached ||
                  engine.state == ChargeStateChargeSuppressed ||
                  engine.state == ChargeStateFault)) {
                violation_reason = "Suppression active in unexpected state";
            }
        }
        
        if (violation_reason != NULL) {
            violations++;
            if (violations <= 10) {
                printf("\n  [FAIL] Invariant Violation #%u at iteration %d (t=%lums):\n", violations, i, (unsigned long)current_time_ms);
                printf("         Reason:  %s\n", violation_reason);
                printf("         State:   %s, Suppressed: %s\n", charge_policy_state_name(engine.state), mock_charger_is_suppressed() ? "YES" : "NO");
                printf("         Inputs:  SOC=%u%%, V=%.2fV, T=%.1fC, USB=%s, Gauge=%s\n", soc, (double)voltage, (double)temp, usb ? "YES" : "NO", gauge ? "OK" : "FAULT");
                printf("         Policy:  Target=%u%%\n", engine.policy.target_soc);
            }
        }
    }
    
    if (violations == 0) {
        printf("  [INFO] 100,000 / 100,000 transitions verified with 0 violations ... ");
        TEST_PASS();
    } else {
        printf("  [FAIL] %u invariant violations detected in 100,000 transitions ... ", violations);
        TEST_FAIL("Safety invariant failure in 100k fuzz loop");
    }
}

void run_charge_policy_tests(void) {
    TEST_SUITE("Phase 2B Charge Policy Engine & Safety Machine");
    test_policy_full();
    test_policy_balanced();
    test_policy_lifespan();
    test_policy_custom();
    test_policy_hysteresis_oscillation();
    test_policy_target_reached();
    test_policy_target_recovery();
    test_policy_usb_disconnect();
    test_policy_invalid_soc();
    test_policy_invalid_voltage();
    test_policy_invalid_temperature();
    test_policy_gauge_failure();
    test_policy_charger_rejection();
    test_policy_charger_disappearance();
    test_policy_controller_restart();
    test_policy_change_during_charge();
    test_policy_fault_recovery();
    test_policy_deterministic_replay();
    test_policy_bounded_memory();
    test_policy_fuzzing_100k();
}
