# Battery Guardian — Comprehensive Test Matrix

This matrix documents the entire test suite supporting Battery Guardian (`v1.0.0-rc1`). Every test case corresponds to an automated check executed by `python tests/run_tests.py` or the unified validation runner `scripts/validate.py`.

**Total Executed Tests:** 118 Unit & Integration Tests  
**Property Testing:** 100,000 Randomized Invariant Transitions  
**Overall Result:** 118 / 118 PASS (0 Failures, 0 Regressions)

---

## 1. Phase 1: Core Pipeline & Telemetry Ingestion (23 Tests)

| ID | Suite | File | Purpose | Expected Result | Status |
|---|---|---|---|---|:---:|
| **P1-TEL-01** | Telemetry Validation | `tests/telemetry/test_validation.c` | Plausible voltage ranges (3.0V - 4.5V) | Flagged `VALID_VOLTAGE` | PASS |
| **P1-TEL-02** | Telemetry Validation | `tests/telemetry/test_validation.c` | Hostile out-of-range voltages (<2.5V, >5.0V) | Rejected / Flag unset | PASS |
| **P1-TEL-03** | Telemetry Validation | `tests/telemetry/test_validation.c` | Temperature validity (-20°C to 60°C) | Flagged `VALID_TEMPERATURE` | PASS |
| **P1-TEL-04** | Telemetry Validation | `tests/telemetry/test_validation.c` | Extreme thermal excursion detection | Flagged as excursion | PASS |
| **P1-SAM-01** | Telemetry Sampling | `tests/telemetry/test_sampling.c` | Adaptive sampling rate while charging | Interval locked to 5000ms | PASS |
| **P1-SAM-02** | Telemetry Sampling | `tests/telemetry/test_sampling.c` | Adaptive sampling rate while idle/active | Interval locked to 10s/30s | PASS |
| **P1-BUF-01** | Telemetry Buffer | `tests/telemetry/test_buffer.c` | Ring buffer FIFO insertion | In-order sample retrieval | PASS |
| **P1-BUF-02** | Telemetry Buffer | `tests/telemetry/test_buffer.c` | Buffer saturation overflow | Oldest evicted, count bounded | PASS |
| **P1-JRN-01** | Journal Records | `tests/journal/test_records.c` | Header magic and version serialization | Magic `0x42474A31`, Ver 1 | PASS |
| **P1-JRN-02** | Journal Records | `tests/journal/test_records.c` | Fixed-size record packing & unpacking | Byte-exact reconstruction | PASS |
| **P1-CRC-01** | Journal Integrity | `tests/journal/test_crc.c` | Per-record CRC32 generation | CRC matches IEEE 802.3 | PASS |
| **P1-CRC-02** | Journal Integrity | `tests/journal/test_crc.c` | Single-bit corruption detection | Record flagged invalid | PASS |
| **P1-REC-01** | Journal Recovery | `tests/journal/test_recovery.c` | Clean file iteration on mount | All valid records loaded | PASS |
| **P1-REC-02** | Journal Recovery | `tests/journal/test_recovery.c` | Mid-file byte corruption isolation | Recovery reads up to corrupt | PASS |
| **P1-SES-01** | Session State | `tests/session/test_state_machine.c` | Charge $\to$ Discharge transition | New session spawned | PASS |
| **P1-SES-02** | Session State | `tests/session/test_state_machine.c` | Short transient jitter suppression | Session preserved | PASS |
| **P1-SES-03** | Session State | `tests/session/test_state_machine.c` | Power unplug event | Immediate session close | PASS |
| **P1-MET-01** | Session Metrics | `tests/session/test_metrics.c` | Coulombic delta accumulation | Accumulated mAh accurate | PASS |
| **P1-MET-02** | Session Metrics | `tests/session/test_metrics.c` | Peak current & temperature tracking | Max/Min captured | PASS |
| **P1-EVT-01** | Event Engine | `tests/event/test_generation.c` | Charge start/stop event emit | Correct event codes queued | PASS |
| **P1-EVT-02** | Event Engine | `tests/event/test_generation.c` | Temperature alert event threshold | Alert event dispatched | PASS |
| **P1-INT-01** | End-to-End Pipeline | `tests/integration/test_pipeline.c` | Telemetry $\to$ Journal $\to$ Model pipeline | Unbroken signal delivery | PASS |
| **P1-INT-02** | End-to-End Pipeline | `tests/integration/test_pipeline.c` | Cold reboot state recovery | Journal state restored | PASS |

---

## 2. Phase 2A: Health & Capacity Intelligence (39 Tests)

| ID | Suite | File | Purpose | Expected Result | Status |
|---|---|---|---|---|:---:|
| **P2A-EST-01** | Estimator Engine | `tests/phase2/test_phase2.c` | Ideal 2100 mAh discharge ($\Delta\text{SOC}=50\%$) | Estimated capacity ~2100 mAh | PASS |
| **P2A-EST-02** | Estimator Engine | `tests/phase2/test_phase2.c` | Degraded 1600 mAh discharge | Estimated capacity ~1600 mAh | PASS |
| **P2A-EST-03** | Estimator Engine | `tests/phase2/test_phase2.c` | Cold temperature capacity suppression | Excursion flag set, dropped | PASS |
| **P2A-EST-04** | Estimator Engine | `tests/phase2/test_phase2.c` | Disqualification on $\Delta\text{SOC} < 15\%$ | Candidate rejected | PASS |
| **P2A-EST-05** | Estimator Engine | `tests/phase2/test_phase2.c` | Disqualification on short duration (<60s) | Candidate rejected | PASS |
| **P2A-OUT-01** | Outlier Filter | `tests/phase2/test_phase2.c` | Single spike outlier insertion | Filtered by median | PASS |
| **P2A-OUT-02** | Outlier Filter | `tests/phase2/test_phase2.c` | Bounded history ring (16 sessions) | Bounded memory ceiling | PASS |
| **P2A-OUT-03** | Outlier Filter | `tests/phase2/test_phase2.c` | Monotonic aging trend survival | Trend preserved | PASS |
| **P2A-OUT-04** | Outlier Filter | `tests/phase2/test_phase2.c` | Zero variance stable series | Exact value maintained | PASS |
| **P2A-OUT-05** | Outlier Filter | `tests/phase2/test_phase2.c` | Alternating high/low candidate noise | Median convergence | PASS |
| **P2A-OUT-06** | Outlier Filter | `tests/phase2/test_phase2.c` | Single candidate initialization | Initial value accepted | PASS |
| **P2A-DAT-01** | Dataset Evaluation | `tests/phase2/test_phase2.c` | Synthetic pristine profile processing | Health ~100% | PASS |
| **P2A-DAT-02** | Dataset Evaluation | `tests/phase2/test_phase2.c` | Synthetic aged profile processing | Health ~75% | PASS |
| **P2A-DAT-03** | Dataset Evaluation | `tests/phase2/test_phase2.c` | High internal resistance profile | Voltage dips rejected | PASS |
| **P2A-DAT-04** | Dataset Evaluation | `tests/phase2/test_phase2.c` | Interrupted discharge profile | Disqualified cleanly | PASS |
| **P2A-ADV-01** | Adversarial Tests | `tests/phase2/test_phase2.c` | Zero-duration session processing | Rejected without divide-by-0 | PASS |
| **P2A-ADV-02** | Adversarial Tests | `tests/phase2/test_phase2.c` | Negative timestamp rollover in session | Disqualified cleanly | PASS |
| **P2A-ADV-03** | Adversarial Tests | `tests/phase2/test_phase2.c` | Zero accumulated energy session | Disqualified cleanly | PASS |
| **P2A-ADV-04** | Adversarial Tests | `tests/phase2/test_phase2.c` | Corrupt estimate payload memory buffer | Engine does not crash | PASS |
| **P2A-ADV-05** | Adversarial Tests | `tests/phase2/test_phase2.c` | Severe thermal excursion session | Disqualified cleanly | PASS |
| **P2A-CNF-01** | Confidence Scoring | `tests/phase2/test_phase2.c` | Empty history confidence query | Score = `INSUFFICIENT` | PASS |
| **P2A-CNF-02** | Confidence Scoring | `tests/phase2/test_phase2.c` | 6 tight qualifying candidates | Score = `MEDIUM` or `HIGH` | PASS |
| **P2A-CNF-03** | Confidence Scoring | `tests/phase2/test_phase2.c` | High variance candidate spread | Score downgraded to `LOW` | PASS |
| **P2A-CNF-04** | Confidence Scoring | `tests/phase2/test_phase2.c` | Natural language explanation string | Non-empty descriptive string | PASS |
| **P2A-STL-01** | Model Freshness | `tests/phase2/test_phase2.c` | Query 1 day after update | Status = `FRESH` | PASS |
| **P2A-STL-02** | Model Freshness | `tests/phase2/test_phase2.c` | Query 45 days after update | Status = `STALE` | PASS |
| **P2A-SCL-01** | 10,000 Session Scale | `tests/phase2/test_phase2.c` | Memory footprint after 10,000 sessions | Ring buffer strictly 16 items | PASS |
| **P2A-SCL-02** | 10,000 Session Scale | `tests/phase2/test_phase2.c` | Robust estimate after 10,000 sessions | Plausible within 1800±50 mAh | PASS |
| **P2A-INL-01..11**| Inline Assertions | `tests/phase2/test_phase2.c` | Math & bounds checks across pipeline | All invariants verified | PASS |

---

## 3. Phase 2A: Hardware-Less Synthetic Datasets (10 Tests)

| ID | Suite | File | Purpose | Expected Result | Status |
|---|---|---|---|---|:---:|
| **SIM-01** | Synthetic Datasets | `tests/simulation/test_simulation.c` | Nominal stable battery profile | Healthy capacity converged | PASS |
| **SIM-02** | Synthetic Datasets | `tests/simulation/test_simulation.c` | Linear capacity degradation profile | Slope tracked correctly | PASS |
| **SIM-03** | Synthetic Datasets | `tests/simulation/test_simulation.c` | Single anomalous spike discharge | Outlier rejected by median | PASS |
| **SIM-04** | Synthetic Datasets | `tests/simulation/test_simulation.c` | Shallow partial discharge session | Skipped (<15% threshold) | PASS |
| **SIM-05** | Synthetic Datasets | `tests/simulation/test_simulation.c` | Rapidly interrupted discharge cycle | Disqualified without error | PASS |
| **SIM-06** | Synthetic Datasets | `tests/simulation/test_simulation.c` | Deterministic replay consistency | Identical bitwise output | PASS |
| **SIM-07** | Synthetic Datasets | `tests/simulation/test_simulation.c` | Dynamic ambient temperature swings | Excursions flagged | PASS |
| **SIM-08** | Synthetic Datasets | `tests/simulation/test_simulation.c` | Fuel gauge vs ADC disagreement | Safe fallback handled | PASS |
| **SIM-09** | Synthetic Datasets | `tests/simulation/test_simulation.c` | Hostile/pathological telemetry stream | Zero crash / Graceful drop | PASS |
| **SIM-10** | Synthetic Datasets | `tests/simulation/test_simulation.c` | 10,000 session continuous stress | Zero memory leak, bounded | PASS |

---

## 4. Phase 2B: Charge Policy Engine & Safety FSM (20 Tests + 100k Fuzz)

| ID | Suite | File | Purpose | Expected Result | Status |
|---|---|---|---|---|:---:|
| **P2B-POL-01** | Charge Policy | `tests/simulation/test_charge_policy.c` | `FULL` policy (100% ceiling) | Charges to 100% | PASS |
| **P2B-POL-02** | Charge Policy | `tests/simulation/test_charge_policy.c` | `BALANCED` policy (80% ceiling) | Suppresses at 80% | PASS |
| **P2B-POL-03** | Charge Policy | `tests/simulation/test_charge_policy.c` | `LIFESPAN` policy (60% ceiling) | Suppresses at 60% | PASS |
| **P2B-POL-04** | Charge Policy | `tests/simulation/test_charge_policy.c` | `CUSTOM` policy arbitrary ceiling | Honors custom parameters | PASS |
| **P2B-POL-05** | Charge Policy | `tests/simulation/test_charge_policy.c` | Hysteresis prevention | No rapid toggling at limit | PASS |
| **P2B-POL-06** | Charge Policy | `tests/simulation/test_charge_policy.c` | Target reached transition | State = `TARGET_REACHED` | PASS |
| **P2B-POL-07** | Charge Policy | `tests/simulation/test_charge_policy.c` | Drop below hysteresis floor | Resumes charging | PASS |
| **P2B-POL-08** | Charge Policy | `tests/simulation/test_charge_policy.c` | USB disconnect event | Forces `UNMANAGED`, unsuppresses | PASS |
| **P2B-SAF-01** | Safety Lockout | `tests/simulation/test_charge_policy.c` | Invalid/impossible SOC (>100%) | `SAFETY_LOCKOUT` | PASS |
| **P2B-SAF-02** | Safety Lockout | `tests/simulation/test_charge_policy.c` | Pack voltage out-of-bounds (>4.5V) | `SAFETY_LOCKOUT` | PASS |
| **P2B-SAF-03** | Safety Lockout | `tests/simulation/test_charge_policy.c` | Thermal excursion (<0°C or >45°C) | `SAFETY_LOCKOUT` | PASS |
| **P2B-SAF-04** | Safety Lockout | `tests/simulation/test_charge_policy.c` | Fuel gauge I2C communication fault | `SAFETY_LOCKOUT` | PASS |
| **P2B-SAF-05** | Safety Lockout | `tests/simulation/test_charge_policy.c` | Charger command refusal by HAL | Transitions to safe fallback | PASS |
| **P2B-SAF-06** | Safety Lockout | `tests/simulation/test_charge_policy.c` | Disappearance of charger capability | Unmanaged fallback | PASS |
| **P2B-FSM-01** | State Machine | `tests/simulation/test_charge_policy.c` | Controller cold start state | Initialized to `UNMANAGED` | PASS |
| **P2B-FSM-02** | State Machine | `tests/simulation/test_charge_policy.c` | Dynamic policy swap while charging | Evaluates new threshold | PASS |
| **P2B-FSM-03** | State Machine | `tests/simulation/test_charge_policy.c` | Fault clearance & recovery flow | Recovers only when valid | PASS |
| **P2B-FSM-04** | State Machine | `tests/simulation/test_charge_policy.c` | Deterministic replay consistency | Identical transition log | PASS |
| **P2B-FSM-05** | State Machine | `tests/simulation/test_charge_policy.c` | Bounded memory during execution | 0 dynamic allocations | PASS |
| **P2B-FUZ-01** | Property Fuzzing | `tests/simulation/test_charge_policy.c` | 100,000 chaotic input transitions | 0 invariant violations | PASS |

> *Note on Property Fuzzing:* Test case **P2B-FUZ-01** executes 100,000 pseudorandom state transitions verifying that safety lockouts are never bypassed, memory remains constant, and invalid transitions cannot occur.

---

## 5. Phase 3: Storage & Numerical Hardening (6 Tests)

| ID | Suite | File | Purpose | Expected Result | Status |
|---|---|---|---|---|:---:|
| **P3-JRN-01** | Journal Hardening | `tests/hardening/test_journal_hardening.c`| Journal recovery on files >64 KB | 64-bit offsets; no truncation | PASS |
| **P3-JRN-02** | Journal Hardening | `tests/hardening/test_journal_hardening.c`| Strict 512 KB quota limit enforcement | Drops records at 524,288 bytes | PASS |
| **P3-JRN-03** | Journal Hardening | `tests/hardening/test_journal_hardening.c`| Future schema version rejection | Rejects version 99 safely | PASS |
| **P3-NUM-01** | Numerical Hardening| `tests/hardening/test_hostile_telemetry.c`| Division-by-zero guards in all engines | 0.0f reference capacity safe | PASS |
| **P3-NUM-02** | Numerical Hardening| `tests/hardening/test_hostile_telemetry.c`| Extreme float input validation (NaN, Inf) | Rejects hostile floats | PASS |
| **P3-NUM-03** | Numerical Hardening| `tests/hardening/test_hostile_telemetry.c`| 32-bit tick wrap-around (~49.7 days) | Monotonic 64-bit time preserved | PASS |

---

## 6. Permanent Regression Suite (REG_01 - REG_05) (5 Tests)

| ID | Suite | File | Historical Bug Guarded | Expected Result | Status |
|---|---|---|---|---|:---:|
| **REG-01** | Regression Suite | `tests/regression/test_regressions.c` | `SESSION_QUALITY_NO_TEMP_EXCURSION` not initialized | Initialized to true on start | PASS |
| **REG-02** | Regression Suite | `tests/regression/test_regressions.c` | Implicit 16-bit cast truncating >64KB journal | 64-bit file offset preserved | PASS |
| **REG-03** | Regression Suite | `tests/regression/test_regressions.c` | Floating-point divide by zero on 0 mAh | Returns 0.0f, no NaN | PASS |
| **REG-04** | Regression Suite | `tests/regression/test_regressions.c` | `RECORD_STORAGE` closed prematurely | Storage handle held for lifecycle | PASS |
| **REG-05** | Regression Suite | `tests/regression/test_regressions.c` | USB unplug didn't clear charge suppression | Immediately unsuppresses | PASS |

---

## 7. Phase 4: Platform Adapter & Hardware HAL (8 Tests)

| ID | Suite | File | Purpose | Expected Result | Status |
|---|---|---|---|---|:---:|
| **P4-HAL-01** | Platform Adapter | `tests/platform/test_platform_adapter.c` | Normalized SI telemetry units | Volts, Amperes, Celsius valid | PASS |
| **P4-HAL-02** | Platform Adapter | `tests/platform/test_platform_adapter.c` | Flipper F7 platform capabilities query | Returns full F7 bitmask | PASS |
| **P4-HAL-03** | Platform Adapter | `tests/platform/test_platform_adapter.c` | Custom adapter backend injection | Overrides default platform HAL | PASS |
| **P4-HAL-04** | Platform Adapter | `tests/platform/test_platform_adapter.c` | Sensor fault / communication drop | Transitions to `StateFault` | PASS |
| **P4-HAL-05** | Charger HAL | `tests/platform/test_platform_adapter.c` | Charger capability bitmask query | Reports mock or prod caps | PASS |
| **P4-HAL-06** | Charger HAL | `tests/platform/test_platform_adapter.c` | Passive fail-closed production contract | Rejects active register writes | PASS |
| **P4-HAL-07** | Charger HAL | `tests/platform/test_platform_adapter.c` | Charger with missing control capability | Safe rejection, no fault | PASS |
| **P4-HAL-08** | Multi-Phase Scenario | `tests/platform/test_platform_adapter.c` | Boot $\to$ USB $\to$ Chg $\to$ Excursion $\to$ Recovery | Exact state sequence verified | PASS |

---

## 8. Phase 5: Security Audit & Adversarial Persistence (7 Tests)

| ID | Suite | File | Purpose | Expected Result | Status |
|---|---|---|---|---|:---:|
| **SEC-01** | Security Audit | `tests/hardening/test_security_audit.c` | Mutex deadlock elimination in `journal_free` | Clean release without deadlock | PASS |
| **SEC-02** | Security Audit | `tests/hardening/test_security_audit.c` | Oversized (>1024) / invalid record rejection | Rejected before buffer copy | PASS |
| **SEC-03** | Security Audit | `tests/hardening/test_security_audit.c` | Adversarial 0xFFFF length journal fixture | Safely dropped, no heap overflow | PASS |
| **SEC-04** | Security Audit | `tests/hardening/test_security_audit.c` | Corrupted bitflip CRC record rejection | Skipped during recovery | PASS |
| **SEC-05** | Security Audit | `tests/hardening/test_security_audit.c` | Diagnostic counters saturation test | Saturates at `UINT32_MAX`, no wrap | PASS |
| **SEC-06** | Security Audit | `tests/hardening/test_security_audit.c` | Out-of-range custom policy parameters | Clamped to safe defaults | PASS |
| **SEC-07** | Security Audit | `tests/hardening/test_security_audit.c` | Exhaustive null pointer defensive guards | Functions return false safely | PASS |

---

## Reconciliation Summary

$$\text{Phase 1 (23)} + \text{Phase 2A (39)} + \text{Simulation (10)} + \text{Phase 2B (20)} + \text{Phase 3 (6)} + \text{Regressions (5)} + \text{Phase 4 (8)} + \text{Phase 5 (7)} = \mathbf{118\text{ Tests}}$$
$$\mathbf{118 / 118\text{ PASS}}$$
$$\mathbf{100,000 / 100,000\text{ Property Fuzz Transitions PASS}}$$
