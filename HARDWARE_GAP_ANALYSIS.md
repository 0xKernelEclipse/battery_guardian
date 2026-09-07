# Battery Guardian — Hardware Gap Analysis & Subsystem Verification Audit

**Release Target:** Battery Guardian `v1.0.0-rc1`  
**Physical Bench Status:** **NOT PERFORMED**  
**Production Charger Mode:** **PASSIVE / FAIL-CLOSED**

This document establishes the exact boundary between what has been mathematically, programmatically, and mock-verified in software versus what strictly requires physical hardware bench instrumentation.

---

## 1. Subsystem Verification Classification

Every subsystem and hardware interaction is classified into one of four evidential categories:
1. **SOFTWARE VERIFIED:** Logic, math, algorithms, or state machines proven by unit tests, property fuzzing, and static analysis without external hardware dependencies.
2. **PLATFORM API VERIFIED:** Usage of Flipper SDK C APIs audited against public official SDK headers (`furi.h`, `furi_hal_power.h`, `storage.h`, `gui.h`) and validated via clean ARM compilation (`ufbt`).
3. **MOCK VERIFIED:** Platform behavior verified against deterministic simulated mock interfaces under host testing.
4. **PHYSICAL VALIDATION REQUIRED:** Actions that interact with real physical electricity, Li-ion chemistry, PMIC I2C registers, or thermal sensors, requiring bench instrumentation.

---

## 2. Hardware Subsystem Audit Matrix

| Subsystem / Feature | Software Status | Physical Status | Evidential Category | Notes / Safety Boundaries |
|---|---|---|---|---|
| **Telemetry Unit Normalization** | Verified (SI units) | Tested via mock | **SOFTWARE VERIFIED** | Validates Volts, Amperes, °C ranges. |
| **Monotonic 64-bit Timekeeping** | Verified (Rollover) | Tested via mock | **SOFTWARE VERIFIED** | Resilient across 32-bit tick rollover (~49.7 days). |
| **Flipper Power API Consumption** | Verified in `ufbt` | Not tested live | **PLATFORM API VERIFIED** | Audited against Flipper SDK API 87.1. |
| **Fuel Gauge I2C Communication** | Verified via mock | Not tested live | **MOCK VERIFIED / PHYSICAL VALIDATION REQUIRED** | Relies on `furi_hal_power_gauge_is_ok()`. |
| **Coulombic Energy Integration** | Verified (Math) | Not calibrated to cell | **SOFTWARE VERIFIED / PHYSICAL DATA REQUIRED** | $\int I \, dt$ proven accurate mathematically; cell chemistry calibration requires physical cycles. |
| **Median Outlier Filtering** | Verified (16-ring) | Not tested live | **SOFTWARE VERIFIED** | Robust against synthetic noise spikes. |
| **OLS Linear Degradation Slope** | Verified (Math) | Not tested live | **SOFTWARE VERIFIED / PHYSICAL DATA REQUIRED** | Slope math verified; real cell aging requires months of physical telemetry. |
| **Binary Journal SD Storage** | Verified (Mock FS) | Not tested on physical SD | **MOCK VERIFIED / PHYSICAL VALIDATION REQUIRED** | Verified in RAM FS; physical FAT/exFAT SD card performance requires bench run. |
| **512 KB Journal Quota Cap** | Verified (Hard limit) | Not tested live | **SOFTWARE VERIFIED** | Caps writes at 524,288 bytes. |
| **Charge Policy State Machine** | Verified (100k fuzz) | Not tested live | **SOFTWARE VERIFIED** | All transitions and hysteresis bands mathematically verified. |
| **Charge Suppression Actuation** | Refused in production | Blocked (Fail-Closed) | **MOCK VERIFIED / PHYSICAL VALIDATION REQUIRED** | In production, returns `false` unconditionally; zero PMIC writes occur. |
| **Thermal Safety Cutoff** | FSM Verified | Physical NTC unverified | **SOFTWARE VERIFIED / PHYSICAL VALIDATION REQUIRED** | Software locks out on $<0^\circ\text{C}$ or $>45^\circ\text{C}$; requires bench temperature chamber test. |
| **Voltage Boundary Lockout** | FSM Verified | Physical bench unverified | **SOFTWARE VERIFIED / PHYSICAL VALIDATION REQUIRED** | Software locks out on $<3.0\text{V}$ or $>4.5\text{V}$. |
| **USB Disconnect Fail-Safe** | Verified (State reset) | Physical unplug unverified | **MOCK VERIFIED / PHYSICAL VALIDATION REQUIRED** | Software unsuppresses immediately upon VBUS loss. |
| **UI Rendering & Navigation** | Verified in `ufbt` | Not tested on LCD | **PLATFORM API VERIFIED / PHYSICAL VALIDATION REQUIRED** | 128x64 canvas drawing compiles clean; physical visual inspection required. |

---

## 3. The Physical Hardware Safety Gate

Before any future firmware release enables active charge suppression (`CHARGER_CAP_CHARGE_CONTROL`):
1. **Gate 1 (Software):** 118/118 tests pass, 100,000 fuzz transitions pass, 0 compiler warnings. **[COMPLETED - v1.0.0-rc1]**
2. **Gate 2 (Passive Bench Observation):** Execute protocols **B-01, B-04, B-05, B-06** on physical hardware. Confirm accurate telemetry capture, zero SD card corruption, and proper UI rendering without any charge control actuation. **[PENDING]**
3. **Gate 3 (Controlled Bench Suppression):** Execute protocol **B-03** on physical hardware using an external bench power supply with current limiting ($5\text{V}, 500\text{mA}$) and current probe. Verify that `furi_hal_power_suppress_charge_enter()` physically halts charging current without destabilizing the device. **[PENDING]**
4. **Gate 4 (Thermal Excursion Bench):** Execute protocol **B-02** in an environmental chamber or thermal bench to verify hardware NTC behavior. **[PENDING]**

Under no circumstances should active charge control be deployed in production without satisfying all four gates.
