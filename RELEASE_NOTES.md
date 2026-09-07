# Battery Guardian v1.0.0-rc1 Release Notes

**Release Date:** 2026-09-07  
**Build Target:** Flipper Zero (STM32WB55, Target 7, API 87.1)  
**Binary Artifact:** `dist/battery_guardian-v1.0.0-rc1.fap`  
**SHA-256 Checksum:** `9A39752B998C3E90B117BD6BA4C93BE414653EA9D898A7D2478E2CD3F06FA893`  
**Status:** Release Candidate — Software Ready for External Review  
**Physical Hardware Validation:** NOT PERFORMED  

---

## 1. What Battery Guardian Does

Battery Guardian is a battery health observability, capacity estimation, degradation tracking, and charge policy safety engine for the Flipper Zero. It replaces static lookup tables with continuous real-world telemetry analysis, Coulombic integration, and formal safety state machines.

The application observes physical battery behavior, detects anomalies (e.g. sensor disconnects, gauge degradation, thermal excursions), logs crash-resilient metrics to SD storage, and executes multi-state charge policies in a strictly passive, fail-closed configuration.

---

## 2. Major Features

- **Normalized SI Telemetry Adapter Layer**:
  - Encapsulates platform power APIs (`furi_hal_power.h`) behind a clean boundary.
  - Strict physical SI units: pack voltage (V), pack current (A), cell temperature (°C), state-of-charge (%), monotonic millisecond timestamps.
  - Capability bitmask (`TELEMETRY_CAP_*`) indicating supported hardware telemetry channels.
  - Operational states: `Normal`, `Partial`, `Fault`, and `Unavailable`.

- **Capacity Estimation & Battery Intelligence Engine**:
  - Coulombic integration over qualifying discharge sessions ($\Delta\text{SOC} \ge 15\%$).
  - Median-based outlier rejection filter preventing erroneous readings from polluting the health baseline.
  - Multi-factor confidence scoring (`High`, `Medium`, `Low`, `Insufficient`).
  - Long-term degradation tracking via ordinary least-squares linear regression slope.

- **Crash-Resilient Binary Journal**:
  - Append-only binary log with global header and per-record CRC32 verification.
  - Non-truncating 64-bit file offset handling and streaming chunk fallback for files $>64\text{ KB}$.
  - Strict 512 KB storage quota enforcement with rate-limited log alerts.
  - Version mismatch protection preventing downgrade corruption.

- **Charge Policy & Safety State Machine**:
  - Supported policies: `BALANCED` (80% target), `LIFESPAN` (60% target), `FULL` (100% target), and `CUSTOM`.
  - Hysteresis guards preventing rapid PMIC cycling near target thresholds.
  - Hard sensor lockouts on out-of-bounds voltage ($<3.0\text{V}$ or $>4.5\text{V}$), thermal excursion ($<0^\circ\text{C}$ or $>45^\circ\text{C}$), or fuel gauge communication fault.
  - Fail-safe state transitions: USB disconnect immediately forces unmanaged state with zero suppression.

- **Truthful Embedded UI**:
  - 5 dedicated views: Dashboard, Health & Degradation, History Graph, Sessions Log, Diagnostics & Safety.
  - Transparent state display: `REALITY (WAIT)` on cold boot, `REALITY [FAULT]` on fuel gauge communication failure, `Ctrl: Passive (Safe)` in diagnostics.

---

## 3. Compatibility & Requirements

- **Platform:** Flipper Zero (STM32WB55)
- **Target Architecture:** ARM Cortex-M4 (Target 7)
- **API Version:** 87.1
- **Firmware Support:** Official Release 0.101.x+, Release Candidate firmware, and compatible community forks.
- **Hardware Prerequisites:** MicroSD card formatted as FAT32 / exFAT for persistent journaling.

---

## 4. Installation

1. Connect your Flipper Zero via USB or open the **qFlipper** application.
2. Navigate to **File Manager** $\to$ `SD Card/apps/Tools/`.
3. Copy `battery_guardian.fap` into the `Tools/` folder.
4. On the Flipper Zero, open **Applications** $\to$ **Tools** $\to$ **Battery Guardian**.

---

## 5. Software Validation Summary

| Test Suite | Cases | Passed | Failed |
|---|:---:|:---:|:---:|
| Phase 1: Core Pipeline & Journal | 23 | 23 | 0 |
| Phase 2A: Health & Capacity Intelligence | 39 | 39 | 0 |
| Simulation Datasets (Synthetic Hardware-Less) | 10 | 10 | 0 |
| Phase 2B: Charge Policy & Safety Machine | 20 | 20 | 0 |
| Phase 3: Storage & Numerical Hardening | 6 | 6 | 0 |
| Permanent Regression Suite (REG_01 - REG_05) | 5 | 5 | 0 |
| Phase 4: Platform Adapter & Hardware Abstraction | 8 | 8 | 0 |
| Phase 5: Security Audit & Adversarial Persistence | 7 | 7 | 0 |
| **Total Unit & Integration Tests** | **118** | **118** | **0** |
| Property Fuzz Transitions (Randomized Invariant Fuzzing) | 100,000 | 100,000 | 0 |

---

## 6. Safety & Hardware Boundary

> [!WARNING]
> **Physical Hardware Validation: NOT PERFORMED**
>
> Battery Guardian v1.0.0-rc1 is compiled in a strictly **PASSIVE and FAIL-CLOSED** configuration:
> - The production charger HAL does **not** advertise `CHARGER_CAP_CHARGE_CONTROL`.
> - Zero register writes are performed to the BQ25896 PMIC or hardware charging registers.
> - Hardware calls `furi_hal_power_suppress_charge_enter()` and `furi_hal_power_suppress_charge_exit()` are bypassed in the production binary.
> - Any charge suppression command is refused at the HAL layer, logging a warning and maintaining factory unmanaged charging.
>
> Software simulation and fuzzing prove internal mathematical and state machine invariants, but do NOT prove physical electrical safety. Physical bench validation (documented in `HARDWARE_VALIDATION.md`) must be conducted before active hardware charge control is enabled.

---

## 7. Known Limitations

1. **Passive Charger Control**: Charge suppression cannot physically halt current flow in this release; policy actions are telemetry simulations only.
2. **Current Sign Convention**: Charging current relies on the fuel gauge reading (`furi_hal_power_get_battery_current(FuriHalPowerICFuelGauge)`); certain hardware revisions report zero current during trickle charge.
3. **Session Requirement**: Capacity estimation requires at least 3 qualifying discharge sessions of $\ge 15\%$ SOC drop before an observed health estimate is generated.

---

## 8. Reporting Issues & Contributing

- To report bugs or display anomalies: submit an issue using the [Bug Report Template](.github/ISSUE_TEMPLATE/bug_report.md).
- To report security or safety vulnerabilities: follow the instructions in [SECURITY.md](SECURITY.md).
- To contribute code or review the architecture: consult [DEVELOPMENT.md](DEVELOPMENT.md) and [ARCHITECTURE.md](ARCHITECTURE.md).
