# Battery Guardian — Technical Audit & Engineering Novelty Analysis

## 1. Objectives & Ground Rules

This document presents a technical audit of the architectural mechanisms implemented in Battery Guardian. It evaluates the engineering distinctions of the system without hyperbole. In accordance with rigorous engineering review:
- No claims of "world's first", "unique", or "unprecedented" are made unless external prior art has been exhaustively proven absent.
- Every architectural characteristic is evaluated on technical merits and classified into one of three evidential categories:
  - **SUPPORTED:** Direct evidence exists in the codebase and test suite demonstrating the implemented mechanism.
  - **INFERENCE:** Reasonable technical deduction based on current Flipper Zero firmware (`dev` / `0.101.x`) public architecture, but subject to maintainer verification.
  - **UNKNOWN:** External prior art or ecosystem comparisons that cannot be verified without broader survey.

---

## 2. Technical Evaluation of Key Subsystems

### 2.1 Persistent Binary Telemetry Journal with Frame-Level Integrity
- **Implemented Mechanism:** An append-only binary journal (`storage/journal.c`) storing fixed-size records (`RecordTypeSample`, `RecordTypeSession`, `RecordTypeEvent`, `RecordTypeCheckpoint`) protected by individual 32-bit CRC checksums, a global header magic (`0x42474A31`), 64-bit non-truncating file offsets, and a hard 512 KB quota limit. Corrupt trailing bytes are isolated without invalidating preceding records.
- **Comparison with Stock Ecosystem:** Standard Flipper Zero applications either maintain zero persistent state, log plain text via `FURI_LOG_*`, or use simple `.ini` / `.txt` files parsed with `flipper_format`.
- **Classification:** **SUPPORTED** (in codebase & verified via test suite: `SEC_01`–`SEC_04`, `REG_02`, `test_journal_hardening.c`).

### 2.2 Session-Aware Coulombic Capacity Estimation
- **Implemented Mechanism:** Instead of static voltage table lookups, Battery Guardian identifies discharge session boundaries (`session.c`) based on power state and current draw. Over qualifying sessions ($\Delta\text{SOC} \ge 15\%$, no thermal excursion), the engine calculates deliverable capacity via numerical Coulomb counting ($\int I \, dt$) and applies median filtering over an in-memory 16-entry history ring buffer.
- **Comparison with Stock Ecosystem:** Stock Flipper firmware and community apps query `furi_hal_power_get_pct()` or fuel gauge registers directly, reporting uncalibrated factory table percentages.
- **Classification:** **SUPPORTED** (in codebase & verified via 39 Phase 2A tests and 10 synthetic simulation scenarios).

### 2.3 Multi-Factor Confidence Scoring Model
- **Implemented Mechanism:** A deterministic confidence scoring engine (`phase2/confidence.c`) that outputs categorical confidence (`High`, `Medium`, `Low`, `Insufficient`) based on session count, cumulative SOC span, and statistical dispersion across candidate estimates. When confidence is below threshold, UI explicitly presents `ESTIMATING...` or `REALITY (WAIT)` rather than a dubious percentage.
- **Comparison with Stock Ecosystem:** Embedded battery monitors typically present a raw number regardless of statistical certainty.
- **Classification:** **SUPPORTED** (in codebase & verified via `test_confidence.c` and UI scenes).

### 2.4 Deterministic OLS Degradation Trend Analysis
- **Implemented Mechanism:** Ordinary Least Squares (OLS) linear regression (`phase2/degradation.c`) calculated over chronological session capacity points with guardrails against division by zero on identical timestamps or zero variance. Produces capacity loss rate per session/day.
- **Comparison with Stock Ecosystem:** No existing published FAP performs historical regression on battery capacity degradation.
- **Classification:** **INFERENCE** (supported by codebase implementation, but ecosystem absence is based on survey of public `flipper-application-catalog` entries).

### 2.5 Passive Fail-Closed Charger Abstraction Layer (HAL)
- **Implemented Mechanism:** A strict hardware abstraction layer (`core/charger_hal.c`) separating policy decisions from physical actuation. The production compilation path explicitly omits `CHARGER_CAP_CHARGE_CONTROL` from its reported capabilities, unconditionally returns `false` on charge suppression attempts, and logs warnings. Physical register writes are blocked at compile time.
- **Comparison with Stock Ecosystem:** Protects against unintentional electrical damage or lockups resulting from unverified I2C register writes to the BQ25896 PMIC.
- **Classification:** **SUPPORTED** (in codebase & verified via Phase 4 tests `TEST_PLATFORM_ADAPTER_06` and `07`).

### 2.6 Deterministic Hardware-Less Simulation & Property Fuzzing Harness
- **Implemented Mechanism:** A fully self-contained host-based mock environment (`tests/mocks/`) emulating the Furi core, storage filesystem, and power subsystem. Accompanied by a 100,000-iteration randomized property fuzzer (`tests/simulation/test_charge_policy.c`) that validates safety state machine invariants across chaotic transitions without requiring physical hardware.
- **Comparison with Stock Ecosystem:** Most Flipper applications are tested either directly on hardware or using the qFlipper GUI emulator, with minimal automated host-side state-machine fuzzing.
- **Classification:** **SUPPORTED** (in codebase & verified via host execution of `tests/run_tests.py`).

---

## 3. Prior Art & Distinction Matrix

| Feature / Mechanism | Battery Guardian Implementation | Standard Embedded / FAP Approach | Evidential Classification |
|---|---|---|---|
| **Raw Telemetry SI Normalization** | Adapter converts all ADC/IC units to standard SI ($V, A, ^\circ C, \%)$ | Direct raw register calls scattered across UI code | **SUPPORTED** |
| **Journal Fault Tolerance** | Per-record CRC32 + 64-bit file offset streaming recovery | Single corrupt byte invalidates entire file | **SUPPORTED** |
| **Charge Policy Hysteresis** | Multi-state FSM with configurable hysteresis (1–20%) | Bang-bang thresholding with rapid cycling | **SUPPORTED** |
| **Lockout Protection** | Sensor fault, thermal excursion, or voltage anomaly triggers latching lockout | Silently continues or freezes display | **SUPPORTED** |
| **Ecosystem Architecture** | Pure external FAP without requiring custom firmware forks | Custom firmware forks often required for power tweaks | **INFERENCE** |
| **Memory Ceiling** | Bounded ring buffers, zero dynamic allocation in hot loops | Unbounded memory growth or fragmentation | **SUPPORTED** |

---

## 4. Engineering Trade-offs & Deliberate Limitations

1. **Passive vs. Active Control:**
   - *Choice:* Selected strictly passive fail-closed charger HAL for `v1.0.0-rc1`.
   - *Trade-off:* The app cannot physically enforce charge cutoffs on real hardware yet; it can only alert and simulate until physical bench safety validation is performed.
2. **Computational Footprint:**
   - *Choice:* Floating-point math (`float`) used for Coulombic integration and OLS regression.
   - *Trade-off:* STM32WB55 has a single-precision FPU (ARM Cortex-M4F), making float operations hardware-accelerated. However, double precision is avoided to preserve cycle budgets.
3. **Storage Quota:**
   - *Choice:* Hard 512 KB cap on journal file size.
   - *Trade-off:* Historical capacity records are preserved via in-memory checkpoints, but granular raw samples are dropped once quota is exceeded until log rotation or archival.
