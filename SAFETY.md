# Battery Guardian Safety & Hardware Boundaries

## 1. Safety Philosophy & Core Principle

Manipulating battery charging circuits in embedded consumer hardware carries non-trivial fire, swelling, and thermal runaway hazards. Flipper Zero uses a single-cell Lithium-Polymer (LiPo) battery charged via an on-board Power Management IC (PMIC) and monitored by a Texas Instruments fuel gauge.

Battery Guardian adheres to the following non-negotiable safety principle:

> **No code may command hardware charging suppression or alter charge voltage registers without physical hardware validation on instrumented test benches.**

---

## 2. Hardware Validation Boundary

> [!WARNING]
> **Hardware Validation Status: NOT PERFORMED**
>
> The production binary (`dist/battery_guardian.fap`) operates in a strictly **PASSIVE and FAIL-CLOSED** mode:
> - **Zero register writes** are performed to hardware power registers (`furi_hal_power_*`).
> - The production charger HAL (`core/charger_hal.c`) does **not** advertise `CHARGER_CAP_CHARGE_CONTROL`. If charge control is requested via `charger_hal_request_charge_disable()`, it logs:
>   ```
>   [WARN][BatGuard] PASSIVE FAIL-CLOSED: Physical charge suppression rejected on hardware.
>   ```
>   and returns `false` (safe refusal).
> - The application executes all state machines, policy engines, and user interfaces without any risk of unintended hardware state alteration.
>
> Refer to `HARDWARE_VALIDATION.md` for the full physical bench verification protocol that must be completed before active charge control is ever enabled.

---

## 3. The Fail-Safe State Machine

Even within simulation and passive stubbing, Battery Guardian enforces rigid fail-safe invariants:

1. **Default to Unmanaged**:
   Any unhandled condition, missing hardware interface, or unexpected telemetry state immediately transitions the system to `ChargeStateUnmanaged` with suppression disabled.
2. **USB Disconnection Safety**:
   Removing the USB cable immediately forces `ChargeStateUnmanaged`. The system never attempts charge control on battery power.
3. **Hard Sensor Lockout**:
   - Cell voltage $< 3.0\text{V}$ or $> 4.5\text{V}$
   - Cell temperature $< 0^\circ\text{C}$ or $> 45^\circ\text{C}$
   - Fuel gauge error flag asserted
   Any of these conditions causes an immediate transition to `ChargeStateFault`, logging an alarm and locking out further state progression until stable parameters are observed.
4. **Hysteresis Guard**:
   To prevent rapid toggling of charging circuits near policy thresholds, charging suppression requires hysteresis (e.g. 3–5% drop) before charging may resume.

---

## 4. Prerequisites for Future Real-Hardware Deployment

Before any future release may enable active hardware charge suppression:

1. **Physical Bench Instrumentation**:
   - Continuous verification of charging current and bus voltages using an external digital multimeter and digital storage oscilloscope.
   - Temperature logging using external thermocouples during suppression transitions.
2. **Firmware Compatibility Verification**:
   - Verifying how official Flipper Zero firmware (`furi_hal_power_suppress_charge_enter()` and `furi_hal_power_suppress_charge_exit()`) interacts with sleep states, USB enumeration, and background power domains.
3. **Flipper Community & Upstream Review**:
   - Peer review by core Flipper Devices firmware engineers.
4. **Explicit User Opt-In**:
   - Requiring a multi-step user confirmation dialog explaining experimental hardware risks before active charging control could be toggled in settings.
