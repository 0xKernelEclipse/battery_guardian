#include "charge_policy.h"
#include <string.h>
#include <stdio.h>

void charge_policy_init(ChargePolicyEngine* engine, ChargePolicyType type) {
    memset(engine, 0, sizeof(ChargePolicyEngine));
    engine->state = ChargeStateUnmanaged;
    charge_policy_set_type(engine, type);
    engine->transition_count = 0;
    engine->last_state_change_ms = 0;
    
    // Command trace initial default
    const ChargerHalInterface* hal = charger_hal_get_interface();
    if (hal) {
        hal->set_charge_suppressed(false, charger_hal_get_context());
    }
}

static void transition_to(ChargePolicyEngine* engine, ChargeState new_state, const char* reason, uint64_t timestamp_ms, const BatteryTelemetry* telemetry) {
    if (engine->state != new_state) {
        engine->last_transition.previous_state = engine->state;
        engine->last_transition.new_state = new_state;
        strncpy(engine->last_transition.reason, reason, sizeof(engine->last_transition.reason) - 1);
        engine->last_transition.timestamp_ms = timestamp_ms;
        if (telemetry) {
            memcpy(&engine->last_transition.telemetry, telemetry, sizeof(BatteryTelemetry));
        } else {
            memset(&engine->last_transition.telemetry, 0, sizeof(BatteryTelemetry));
        }

        engine->state = new_state;
        engine->transition_count++;
        engine->last_state_change_ms = timestamp_ms;
        
        // Action bindings
        const ChargerHalInterface* hal = charger_hal_get_interface();
        void* ctx = charger_hal_get_context();
        if (hal) {
            switch (new_state) {
                case ChargeStateUnmanaged:
                case ChargeStateChargingAllowed:
                case ChargeStateRecovery:
                    hal->set_charge_suppressed(false, ctx);
                    break;
                case ChargeStateTargetReached:
                case ChargeStateChargeSuppressed:
                case ChargeStateFault:
                    hal->set_charge_suppressed(true, ctx);
                    break;
            }
        }
    }
}

void charge_policy_set_type(ChargePolicyEngine* engine, ChargePolicyType type) {
    if(!engine) return;
    engine->policy.type = type;
    engine->policy.enabled = true;
    engine->policy.temporary_override = false;
    
    switch (type) {
        case ChargePolicyUnmanaged:
            engine->policy.target_soc = 100;
            engine->policy.allowed_hysteresis = 0;
            engine->policy.enabled = false;
            break;
        case ChargePolicyBalanced:
            engine->policy.target_soc = 80;
            engine->policy.allowed_hysteresis = 3;
            break;
        case ChargePolicyLifespan:
            engine->policy.target_soc = 60;
            engine->policy.allowed_hysteresis = 4;
            break;
        case ChargePolicyFull:
            engine->policy.target_soc = 100;
            engine->policy.allowed_hysteresis = 3;
            break;
        case ChargePolicyCustom:
            // Custom defaults
            engine->policy.target_soc = 85;
            engine->policy.allowed_hysteresis = 5;
            break;
    }
    // Force transition re-evaluation
    engine->state = ChargeStateUnmanaged;
}

void charge_policy_set_custom(ChargePolicyEngine* engine, uint8_t target, uint8_t hysteresis) {
    if(!engine) return;
    
    // Bounds checking on custom parameters: target 50-100%, hysteresis 1-20%
    if (target < 50) target = 50;
    if (target > 100) target = 100;
    if (hysteresis < 1) hysteresis = 1;
    if (hysteresis > 20) hysteresis = 20;
    if (hysteresis >= target) hysteresis = target / 2;
    
    engine->policy.type = ChargePolicyCustom;
    engine->policy.target_soc = target;
    engine->policy.allowed_hysteresis = hysteresis;
    engine->policy.enabled = true;
    engine->state = ChargeStateUnmanaged;
}

void charge_policy_update(ChargePolicyEngine* engine, const ChargerHalInterface* hal, void* hal_ctx, uint64_t timestamp_ms) {
    if(!engine) return;
    
    if (!hal) {
        // Safe default: transition to unmanaged on missing interface
        BatteryTelemetry dummy = { .timestamp_ms = timestamp_ms };
        transition_to(engine, ChargeStateUnmanaged, "HAL_NULL", timestamp_ms, &dummy);
        return;
    }

    uint8_t soc = 0;
    float voltage = 0.0f;
    float temp = 0.0f;
    bool charging = false;
    bool usb_present = false;
    bool gauge_ok = false;

    // Fail-safe rule: If reads fail, move to safe/fault state
    bool ok = hal->get_soc(&soc, hal_ctx) &&
              hal->get_voltage(&voltage, hal_ctx) &&
              hal->get_temperature(&temp, hal_ctx) &&
              hal->is_charging(&charging, hal_ctx) &&
              hal->is_usb_present(&usb_present, hal_ctx) &&
              hal->gauge_ok(&gauge_ok, hal_ctx);

    BatteryTelemetry sample = {
        .timestamp_ms = timestamp_ms,
        .soc_pct = soc,
        .voltage_v = voltage,
        .temperature_c = temp,
        .charging = charging,
        .usb_present = usb_present,
        .gauge_ok = gauge_ok,
        .flags = ok ? 0xFFFFFFFF : 0
    };

    if (!ok) {
        transition_to(engine, ChargeStateFault, "HAL_READ_FAIL", timestamp_ms, &sample);
        return;
    }

    // Fail-safe: USB disconnected forces unmanaged charging configuration immediately
    if (!usb_present) {
        transition_to(engine, ChargeStateUnmanaged, "USB_DISCONNECT", timestamp_ms, &sample);
        return;
    }

    // Fail-safe: Invalid parameters force Fault
    if (!gauge_ok) {
        transition_to(engine, ChargeStateFault, "GAUGE_FAULT", timestamp_ms, &sample);
        return;
    }
    if (voltage < 3.0f || voltage > 4.5f) {
        transition_to(engine, ChargeStateFault, "VOLTAGE_OUT_OF_BOUNDS", timestamp_ms, &sample);
        return;
    }
    if (temp > 45.0f || temp < 0.0f) {
        transition_to(engine, ChargeStateFault, "TEMP_OUT_OF_BOUNDS", timestamp_ms, &sample);
        return;
    }
    if (soc > 100) {
        transition_to(engine, ChargeStateFault, "INVALID_SOC", timestamp_ms, &sample);
        return;
    }

    // If policy is disabled, remain in Unmanaged state
    if (!engine->policy.enabled) {
        transition_to(engine, ChargeStateUnmanaged, "POLICY_DISABLED", timestamp_ms, &sample);
        return;
    }

    uint8_t target = engine->policy.target_soc;
    uint8_t hyst = engine->policy.allowed_hysteresis;

    switch (engine->state) {
        case ChargeStateUnmanaged:
        case ChargeStateRecovery:
            // evaluation point
            if (soc >= target) {
                transition_to(engine, ChargeStateTargetReached, "TARGET_HIT", timestamp_ms, &sample);
            } else {
                transition_to(engine, ChargeStateChargingAllowed, "START_CHARGE", timestamp_ms, &sample);
            }
            break;

        case ChargeStateChargingAllowed:
            if (soc >= target) {
                transition_to(engine, ChargeStateTargetReached, "TARGET_HIT", timestamp_ms, &sample);
            }
            break;

        case ChargeStateTargetReached:
            // Acknowledge command suppression completion
            transition_to(engine, ChargeStateChargeSuppressed, "SUPPRESSION_CONFIRMED", timestamp_ms, &sample);
            break;

        case ChargeStateChargeSuppressed:
            // Hysteresis: resume charge when SOC drops below target - hysteresis
            if (soc <= (target - hyst)) {
                transition_to(engine, ChargeStateChargingAllowed, "HYSTERESIS_RESUME", timestamp_ms, &sample);
            }
            break;

        case ChargeStateFault:
            // Move to recovery to recheck safety metrics
            transition_to(engine, ChargeStateRecovery, "FAULT_CLEAR_TRY", timestamp_ms, &sample);
            break;

        default:
            transition_to(engine, ChargeStateFault, "UNEXPECTED_STATE", timestamp_ms, &sample);
            break;
    }
}

const char* charge_policy_state_name(ChargeState state) {
    switch (state) {
        case ChargeStateUnmanaged:       return "UNMANAGED";
        case ChargeStateChargingAllowed: return "CHARGING_ALLOWED";
        case ChargeStateTargetReached:   return "TARGET_REACHED";
        case ChargeStateChargeSuppressed:return "CHARGE_SUPPRESSED";
        case ChargeStateFault:           return "FAULT";
        case ChargeStateRecovery:        return "RECOVERY";
    }
    return "UNKNOWN";
}

const char* charge_policy_type_name(ChargePolicyType type) {
    switch (type) {
        case ChargePolicyUnmanaged: return "UNMANAGED";
        case ChargePolicyBalanced:  return "BALANCED";
        case ChargePolicyLifespan:  return "LIFESPAN";
        case ChargePolicyFull:      return "FULL";
        case ChargePolicyCustom:    return "CUSTOM";
    }
    return "UNKNOWN";
}
