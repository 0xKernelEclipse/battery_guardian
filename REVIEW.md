# Battery Guardian — External Reviewer Guide

Welcome to the technical review package for **Battery Guardian v1.0.0-rc1**. This document is designed to provide firmware maintainers, embedded systems engineers, and community reviewers with a structured, transparent tour of the codebase, its architectural invariants, and its verification evidence.

---

## 1. What Problem Does Battery Guardian Solve?

Standard embedded battery displays reduce power state to an instantaneous State of Charge percentage ($\text{SOC}\%$). On the Flipper Zero, this percentage is derived from stock fuel gauge lookups that:
1. Suffer from temporary voltage depression under heavy dynamic loads (screen, radio, SD writes).
2. Fail to account for physical cell degradation over time (reporting $100\%$ on a worn cell with only $60\%$ nominal capacity remaining).
3. Provide zero historical trend tracking or degradation slope derivation.
4. Execute unmanaged charging without configurable cycle-preservation ceilings (e.g. 80% or 60% limits).

Battery Guardian bridges this gap by replacing static lookups with **continuous telemetry observation, session-aware Coulombic capacity estimation, statistical confidence rating, and formal charge safety state machines**.

For deep background, see [PROBLEM_STATEMENT.md](PROBLEM_STATEMENT.md).

---

## 2. Why an External FAP?

Flipper's official [Contributing Guidelines](https://github.com/flipperdevices/flipperzero-firmware/blob/dev/CONTRIBUTING.md) explicitly state:
> *"If an idea can be implemented as an external application, it is usually better to implement it as such and publish it in the App Catalog."*

Battery Guardian adheres strictly to this architectural guidance:
- **No Firmware Forks Required:** Operates entirely through public, standard SDK APIs (`furi`, `furi_hal_power`, `storage`, `gui`).
- **Zero Impact on Base OS Footprint:** Consumes flash and RAM only when launched by the user; does not burden core firmware image sizes.
- **Safe Isolation:** Crashes or faults inside an external FAP are contained by Furi OS without destabilizing device radio, RFID, or Sub-GHz subsystems.
- **Upgradability:** Can be iterated and updated independently through the Flipper Apps Catalog without waiting for firmware release cycles.

For architectural trade-offs, see [EXTERNAL_APP_BOUNDARY.md](EXTERNAL_APP_BOUNDARY.md).

---

## 3. Core Architecture

Battery Guardian follows a decoupled, layered pipeline:

```
Physical Hardware (Fuel Gauge & PMIC)
               │
               ▼
┌──────────────────────────────────────┐
│  1. Telemetry Adapter Layer          │ ◄── Enforces normalized SI units (V, A, °C, %)
└──────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────┐
│  2. Telemetry Engine & Ingestion     │ ◄── Adaptive sampling (5s chg, 10s act, 30s idle)
└──────────────────────────────────────┘
        │                     │
        ▼                     ▼
┌──────────────┐      ┌─────────────────┐
│ Binary       │      │ Session Manager │ ◄── Identifies discharge boundaries
│ Journal      │      └─────────────────┘
│ (CRC32 Frame)│              │
└──────────────┘              ▼
                      ┌─────────────────┐
                      │ Coulombic       │ ◄── Integrates current over qualifying
                      │ Estimator       │     windows (ΔSOC ≥ 15%)
                      └─────────────────┘
                              │
                              ▼
                      ┌─────────────────┐
                      │ Health &        │ ◄── Median outlier filter & OLS
                      │ Confidence      │     linear regression
                      └─────────────────┘
                              │
                              ▼
┌──────────────────────────────────────┐
│  3. Charge Policy & Safety FSM       │ ◄── Evaluates Balanced/Lifespan/Custom
└──────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────┐
│  4. Charger HAL (Passive Fail-Closed)│ ◄── Rejects register writes in production
└──────────────────────────────────────┘
```

### Mutex & Threading Invariants
- **Background Worker (`battery_model.c`):** Polling, numerical integration, journal writes, and policy evaluations execute on the background thread *outside* the model mutex.
- **Publish-Only Mutex:** The model mutex is held exclusively for a microsecond snapshot copy (`battery_model_get_snapshot()`), ensuring UI rendering threads never block on storage I/O.
- **Journal Mutex Hierarchy:** Re-entrant deadlocks are eliminated by extracting `journal_flush_locked()` for internal callers, while the public `journal_flush()` acquires the lock once.

For complete threading and locking specifications, see [ARCHITECTURE.md](ARCHITECTURE.md).

---

## 4. Battery Intelligence & Mathematical Modeling

1. **Coulombic Integration:** Over qualifying discharge sessions ($\Delta\text{SOC} \ge 15\%$, duration $> 60\text{s}$, no thermal excursions):
   $$\text{Capacity}_{\text{observed}} = \frac{1}{\Delta\text{SOC}} \int_{t_{\text{start}}}^{t_{\text{end}}} I(t) \, dt$$
2. **Median Outlier Filtering:** New capacity candidates enter a bounded 16-entry ring buffer. The working estimate is derived from the statistical median, immunizing the health metric against single-session anomalies.
3. **Multi-Factor Confidence Scoring:** Evaluates sample count, cumulative SOC span, and statistical variance to output `High`, `Medium`, `Low`, or `Insufficient`.
4. **Degradation Slope:** Computes linear regression slope over chronological session history, reporting estimated capacity loss per session/day.

---

## 5. Safety Model & Fault Recovery

Battery Guardian implements a formal Finite State Machine (`phase2/charge_policy.c`) with 6 operational states:
- `UNMANAGED`: Default unsuppressed state when USB is disconnected or app starts.
- `CHARGING_ALLOWED`: Charging in progress below target threshold.
- `TARGET_REACHED`: Battery has reached target ceiling; suppression requested.
- `CHARGE_SUPPRESSED`: Active suppression state with hysteresis monitoring.
- `SAFETY_LOCKOUT`: Latching error state triggered by out-of-bounds voltage ($<3.0\text{V}$ or $>4.5\text{V}$), thermal excursion ($<0^\circ\text{C}$ or $>45^\circ\text{C}$), or sensor fault.
- `FAULT`: Irrecoverable system condition.

### Key Invariant: USB Disconnect Fail-Safe
If USB is unplugged at any time, the FSM unconditionally forces transition to `UNMANAGED` and clears any suppression state. When USB is later reconnected, factory default charging occurs uninterrupted.

For formal state transition tables, see [SAFETY.md](SAFETY.md).

---

## 6. Hardware Boundary: Fail-Closed Production Contract

> [!IMPORTANT]
> **Physical Hardware Validation Status: NOT PERFORMED**

To guarantee zero electrical risk prior to physical bench validation:
- The production charger HAL (`core/charger_hal.c`, `#else` branch) does **not** advertise `CHARGER_CAP_CHARGE_CONTROL`.
- All charge suppression calls return `false` unconditionally and emit a warning log.
- Zero register writes are performed to the Texas Instruments BQ25896 PMIC.
- The user interface displays `Ctrl: Passive (Safe)` in the Diagnostics view.

Software test evidence proves mathematical correctness and state machine invariants; it does **not** substitute for physical bench testing. See [HARDWARE_GAP_ANALYSIS.md](HARDWARE_GAP_ANALYSIS.md).

---

## 7. Test Evidence

The repository contains **118 unit and integration tests** plus **100,000 property fuzz transitions**:

```
Phase 1 (Core Pipeline & Journal):             23/23 PASS
Phase 2A (Health & Capacity Intelligence):      39/39 PASS
Simulation Datasets (Synthetic Hardware-Less):  10/10 PASS
Phase 2B (Charge Policy & Safety Machine):      20/20 PASS
Phase 3 (Storage & Numerical Hardening):         6/6  PASS
Permanent Regression Suite (REG_01 - REG_05):    5/5  PASS
Phase 4 (Platform Adapter & Hardware HAL):       8/8  PASS
Phase 5 (Security Audit & Adversarial Fuzz):     7/7  PASS
------------------------------------------------------------
Total Unit & Integration Tests:                118/118 PASS
Property Fuzz State Transitions:             100,000 PASS
```

To run the complete host suite:
```bash
python tests/run_tests.py
```

For a comprehensive line-by-line breakdown of every test case, see [TEST_MATRIX.md](TEST_MATRIX.md).

---

## 8. Known Limitations

1. **Passive Charger Control:** In `v1.0.0-rc1`, charging is observed and evaluated, but not physically halted.
2. **Current Sign Convention:** Relies on the BQ27220 fuel gauge ADC reading. During low-current trickle charging, some hardware revisions report near-zero current.
3. **Cold Boot State:** Requires 3 qualifying discharge sessions before displaying an observed health estimate. Prior to that, the UI displays `REALITY (WAIT)` or `ESTIMATING...`.
4. **Storage Quota:** Journal storage is capped at 512 KB to avoid monopolizing SD card space. Once reached, raw sample logging stops until log rotation or user archival.

---

## 9. Questions for Technical Reviewers

We specifically invite feedback on several design choices:
- See [QUESTIONS_FOR_REVIEW.md](QUESTIONS_FOR_REVIEW.md) for concrete questions regarding telemetry sampling cadence, background persistence, and charger HAL design.

---

## 10. Hardware Validation Requirements

Before active hardware charge suppression can be enabled:
- Physical bench testing must be executed using an external power supply, current probe, and oscilloscopes following protocols **B-01 through B-06** in [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md).
- Results must be logged using [HARDWARE_EVIDENCE_TEMPLATE.md](HARDWARE_EVIDENCE_TEMPLATE.md).
