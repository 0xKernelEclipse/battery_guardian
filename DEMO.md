# Battery Guardian — Deterministic Demonstration & Lifecycle Trace

This document presents a complete, deterministic execution trace of Battery Guardian through its entire operational lifecycle: from cold boot, through active discharge, session qualification, Coulombic capacity estimation, USB connection, charge management, emergency thermal lockout, hysteresis cooling recovery, target ceiling cutoff, and USB disconnect fail-safe.

Reviewers can inspect this deterministic scenario to understand exactly how Battery Guardian processes signals and enforces safety without requiring physical hardware.

---

## 1. High-Level Scenario Progression

```
[1. COLD BOOT] (Tick 0)
       │  • Telemetry: 3.92V, -120mA, 23.0°C, 75% SOC
       │  • State: UNMANAGED, REALITY (WAIT)
       ▼
[2. NORMAL DISCHARGE] (Ticks 1000 - 3600000)
       │  • User operates device; SOC drops from 75% to 52% (ΔSOC = 23% ≥ 15%)
       │  • Coulombic energy integrated: 462.4 mAh
       ▼
[3. SESSION COMPLETION & CAPACITY ESTIMATE] (Tick 3600010)
       │  • Discharge session closes cleanly; qualifies for intelligence engine
       │  • Calculated capacity: 462.4 / 0.23 = 2010.4 mAh
       │  • Outlier filter integrates value into 16-session ring buffer
       │  • Confidence score evaluated: HIGH
       ▼
[4. USB CONNECTED & CHARGING ALLOWED] (Tick 3610000)
       │  • VBUS detected (5.02V); USB charging flags true
       │  • Policy: BALANCED (80% target ceiling)
       │  • State transitions: UNMANAGED ──> CHARGING_ALLOWED
       ▼
[5. THERMAL EXCURSION LOCKOUT] (Tick 3640000)
       │  • Ambient heat raises cell temperature to 48.5°C (>45.0°C threshold)
       │  • Safety State Machine triggers emergency override
       │  • State transitions: CHARGING_ALLOWED ──> SAFETY_LOCKOUT
       │  • Actuation: Charge suppression commanded; diagnostics counter incremented
       ▼
[6. COOLING & TWO-STAGE RECOVERY] (Tick 3670000 - 3671000)
       │  • Cell temperature cools to 34.0°C (<40.0°C hysteresis band)
       │  • Stage 1: Transition to RECOVERY (validates telemetry stability)
       │  • Stage 2: Transition back to CHARGING_ALLOWED; suppression released
       ▼
[7. TARGET CEILING REACHED] (Tick 3750000)
       │  • Battery charges to 81% SOC (>80% BALANCED ceiling)
       │  • State transitions: CHARGING_ALLOWED ──> TARGET_REACHED ──> CHARGE_SUPPRESSED
       │  • Suppression commanded to preserve battery cycle life
       ▼
[8. USB DISCONNECT FAIL-SAFE] (Tick 3760000)
       │  • USB cable unplugged; VBUS collapses to 0.0V
       │  • State Machine resets immediately: CHARGE_SUPPRESSED ──> UNMANAGED
       │  • Suppression flag cleared; ready for factory default charging upon next connect
```

---

## 2. Chronological Machine-Readable Trace Log

```json
[
  {
    "step": 1,
    "event": "BOOT_INITIALIZE",
    "timestamp_ms": 1000,
    "telemetry": { "voltage_v": 3.92, "current_a": -0.120, "temperature_c": 23.0, "soc_pct": 75, "usb_present": false, "charging": false, "gauge_ok": true },
    "fsm_state": "ChargeStateUnmanaged",
    "suppression_active": false,
    "observed_capacity_mah": 0.0,
    "confidence": "Insufficient",
    "ui_reality_header": "REALITY (WAIT)"
  },
  {
    "step": 2,
    "event": "DISCHARGE_IN_PROGRESS",
    "timestamp_ms": 1800000,
    "telemetry": { "voltage_v": 3.75, "current_a": -0.145, "temperature_c": 24.2, "soc_pct": 63, "usb_present": false, "charging": false, "gauge_ok": true },
    "fsm_state": "ChargeStateUnmanaged",
    "suppression_active": false,
    "session_active": true,
    "session_delta_soc": 12,
    "session_accumulated_mah": 241.2
  },
  {
    "step": 3,
    "event": "SESSION_QUALIFIED_CLOSED",
    "timestamp_ms": 3600000,
    "telemetry": { "voltage_v": 3.65, "current_a": -0.050, "temperature_c": 23.5, "soc_pct": 52, "usb_present": false, "charging": false, "gauge_ok": true },
    "fsm_state": "ChargeStateUnmanaged",
    "session_completed": { "duration_s": 3600, "delta_soc": 23, "energy_mah": 462.4, "candidate_mah": 2010.4 },
    "estimator_update": { "median_capacity_mah": 2008.5, "nominal_mah": 2100.0, "health_pct": 95.6, "confidence": "High" },
    "ui_reality_header": "REALITY 95%"
  },
  {
    "step": 4,
    "event": "USB_INSERTED",
    "timestamp_ms": 3610000,
    "telemetry": { "voltage_v": 3.78, "current_a": 0.450, "temperature_c": 24.0, "soc_pct": 52, "usb_present": true, "charging": true, "gauge_ok": true },
    "policy_selected": "BALANCED",
    "target_soc": 80,
    "fsm_state": "ChargeStateChargingAllowed",
    "suppression_active": false,
    "action": "ALLOW_CHARGING"
  },
  {
    "step": 5,
    "event": "THERMAL_EXCURSION_TRIGGER",
    "timestamp_ms": 3640000,
    "telemetry": { "voltage_v": 3.90, "current_a": 0.420, "temperature_c": 48.5, "soc_pct": 68, "usb_present": true, "charging": true, "gauge_ok": true },
    "fsm_state": "ChargeStateFault",
    "suppression_active": true,
    "action": "COMMAND_SUPPRESS_CHARGE",
    "diagnostic_event": "THERMAL_LOCKOUT_LATCHED",
    "ui_reality_header": "REALITY [FAULT]"
  },
  {
    "step": 6,
    "event": "THERMAL_HYSTERESIS_RECOVERY",
    "timestamp_ms": 3670000,
    "telemetry": { "voltage_v": 3.88, "current_a": 0.010, "temperature_c": 34.0, "soc_pct": 68, "usb_present": true, "charging": false, "gauge_ok": true },
    "fsm_state": "ChargeStateRecovery",
    "suppression_active": true,
    "action": "EVALUATING_STABILITY"
  },
  {
    "step": 7,
    "event": "RECOVERY_CONFIRMED",
    "timestamp_ms": 3671000,
    "telemetry": { "voltage_v": 3.88, "current_a": 0.440, "temperature_c": 33.8, "soc_pct": 68, "usb_present": true, "charging": true, "gauge_ok": true },
    "fsm_state": "ChargeStateChargingAllowed",
    "suppression_active": false,
    "action": "RESUME_CHARGING"
  },
  {
    "step": 8,
    "event": "TARGET_CEILING_REACHED",
    "timestamp_ms": 3750000,
    "telemetry": { "voltage_v": 4.12, "current_a": 0.380, "temperature_c": 28.5, "soc_pct": 81, "usb_present": true, "charging": true, "gauge_ok": true },
    "fsm_state": "ChargeStateChargeSuppressed",
    "suppression_active": true,
    "action": "COMMAND_SUPPRESS_CHARGE",
    "ui_status": "TARGET REACHED (80%)"
  },
  {
    "step": 9,
    "event": "USB_DISCONNECTED",
    "timestamp_ms": 3760000,
    "telemetry": { "voltage_v": 4.08, "current_a": -0.090, "temperature_c": 27.0, "soc_pct": 80, "usb_present": false, "charging": false, "gauge_ok": true },
    "fsm_state": "ChargeStateUnmanaged",
    "suppression_active": false,
    "action": "RESET_FAILSAFE",
    "ui_status": "DISCHARGING"
  }
]
```

---

## 3. Reviewer Verification Instructions

This trace is programmatically executed and asserted by **Test Case P4-HAL-08** in `tests/platform/test_platform_adapter.c`. To replay and verify this exact sequence locally:
```bash
python tests/run_tests.py
```
The test suite confirms that every state transition, hysteresis hold, and suppression reset matches the expected values bit-for-bit.
