#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../core/telemetry.h"
#include "charger_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ChargeStateUnmanaged,          // Safe default / uncertainty (charging allowed but unsuppressed)
    ChargeStateChargingAllowed,    // Charging actively allowed under policy target
    ChargeStateTargetReached,      // Target SOC hit
    ChargeStateChargeSuppressed,   // Charging suppressed command acknowledged
    ChargeStateFault,              // Lockout state (temperature, voltage, gauge error)
    ChargeStateRecovery            // Fault recovery transition state
} ChargeState;

typedef enum {
    ChargePolicyUnmanaged,
    ChargePolicyBalanced,
    ChargePolicyLifespan,
    ChargePolicyFull,
    ChargePolicyCustom
} ChargePolicyType;

typedef struct {
    ChargePolicyType type;
    uint8_t target_soc;
    uint8_t allowed_hysteresis;
    bool enabled;
    bool temporary_override;
} ChargePolicy;

typedef struct {
    ChargeState previous_state;
    ChargeState new_state;
    char reason[32];
    uint64_t timestamp_ms;
    BatteryTelemetry telemetry;
} ChargeTransition;

typedef struct {
    ChargeState state;
    ChargePolicy policy;
    uint32_t transition_count;
    uint64_t last_state_change_ms;
    ChargeTransition last_transition;
} ChargePolicyEngine;

void charge_policy_init(ChargePolicyEngine* engine, ChargePolicyType type);
void charge_policy_update(ChargePolicyEngine* engine, const ChargerHalInterface* hal, void* hal_ctx, uint64_t timestamp_ms);
void charge_policy_set_custom(ChargePolicyEngine* engine, uint8_t target, uint8_t hysteresis);
void charge_policy_set_type(ChargePolicyEngine* engine, ChargePolicyType type);

const char* charge_policy_state_name(ChargeState state);
const char* charge_policy_type_name(ChargePolicyType type);

#ifdef __cplusplus
}
#endif
