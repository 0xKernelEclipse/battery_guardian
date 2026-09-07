# Battery Guardian — Code Coverage Analysis & Rationale

**Status:** **NOT MEASURED (Host Toolchain Limitation)**  
**Target Architecture:** ARM Cortex-M4F (STM32WB55)  
**Host Toolchain:** Zig 0.13.0 (`zig cc`) targeting Windows x86_64 PE/COFF

---

## 1. Toolchain Measurement Rationale

During the Phase 6 verification audit, compiler instrumentation flags (`-fprofile-instr-generate -fcoverage-mapping` and `--coverage`) were evaluated on the local host development toolchain (`zig cc` on Windows).

### Technical Blocker
Linking an instrumented binary with `zig cc` on Windows produces an unresolved symbol error:
```
lld-link: error: undefined symbol: __llvm_profile_runtime
>>> referenced by (__llvm_profile_runtime_user)
```
The standalone Windows binary distribution of Zig 0.13 does not package the static LLVM profiling runtime archive (`libclang_rt.profile-x86_64.lib`). Similarly, GCC-style `gcov` profiling requires linking `libgcov.a`, which is not available in the portable Clang/LLVM linker path on Windows.

As specified by the Battery Guardian quality guidelines:
> *"If coverage cannot be reliably measured in this toolchain: state NOT MEASURED and explain why. A lower percentage with meaningful boundary testing is preferable to inflated coverage from trivial tests."*

Consequently, automated line/branch percentages are designated as **NOT MEASURED** on this local workstation.

---

## 2. Structural & Subsystem Coverage Inspection

Although toolchain instrumentation cannot emit a `.profdata` report on Windows, manual white-box inspection of the 118 unit tests confirms that critical code paths are thoroughly exercised:

### 2.1 Storage & Binary Journal Parser (`storage/journal.c`)
- **Header Magic & Version Check:** Tested in `P1-JRN-01`, `P3-JRN-03` (rejects future format version 99).
- **CRC32 Frame Calculation & Verification:** Tested in `P1-CRC-01`, `P1-CRC-02`, and adversarial bitflip corruption `SEC_04`.
- **Streaming Recovery & 64-bit Offsets:** Tested in `P1-REC-01`, `P1-REC-02`, `P3-JRN-01`, `REG_02`.
- **512 KB Storage Quota:** Hard boundary enforcement tested in `P3-JRN-02`.
- **Re-entrant Mutex Deadlock Elimination:** Verified in `SEC_01`.
- **Adversarial Buffering (0xFFFF length & garbage bytes):** Verified in `SEC_02`, `SEC_03`.

### 2.2 Telemetry Ingestion & Normalization (`core/telemetry.c`, `core/telemetry_adapter.c`)
- **SI Unit Conversion:** Tested in `P4-HAL-01`.
- **Plausibility Filters (Voltage, Current, Temp):** Tested in `P1-TEL-01`, `P1-TEL-02`, `P1-TEL-03`, `P1-TEL-04`, `P3-NUM-02`.
- **32-bit Tick Rollover (~49.7 days):** Verified in `P3-NUM-03`.
- **Adaptive Sampling State Engine:** Tested in `P1-SAM-01`, `P1-SAM-02`.
- **Sensor Fault Transitions:** Tested in `P4-HAL-04`.

### 2.3 Capacity Estimation & Degradation (`phase2/estimator.c`, `confidence.c`, `degradation.c`)
- **Coulombic Integration ($\int I \, dt$):** Tested in `P2A-EST-01`, `P2A-EST-02`.
- **Session Disqualification (<15% drop, <60s duration):** Tested in `P2A-EST-04`, `P2A-EST-05`.
- **Median Outlier Filtering:** Tested in `P2A-OUT-01` through `P2A-OUT-06`.
- **Confidence Scoring State Matrix:** Tested in `P2A-CNF-01` through `P2A-CNF-04`.
- **Division-by-Zero Guards (Zero reference capacity):** Tested in `P3-NUM-01`, `REG_03`.
- **Scale Stability (10,000 sessions):** Tested in `P2A-SCL-01`, `P2A-SCL-02`.

### 2.4 Charge Policy & Safety State Machine (`phase2/charge_policy.c`)
- **All Policy Modes (BALANCED, LIFESPAN, FULL, CUSTOM):** Tested in `P2B-POL-01` through `P2B-POL-04`.
- **Hysteresis Band Clamping:** Tested in `P2B-POL-05`, `SEC_06`.
- **Exhaustive Lockout Paths (Voltage, Thermal, Gauge):** Tested in `P2B-SAF-01` through `P2B-SAF-04`.
- **Fail-Safe Unmanaged Transitions (USB Unplug):** Tested in `P2B-POL-08`, `REG_05`.
- **Chaotic Property Fuzzing (100,000 transitions):** Tested in `P2B-FUZ-01`.

---

## 3. Recommended CI Coverage Pipeline

For automated collection of line, function, and branch coverage in continuous integration:
1. The GitHub Actions workflow (`.github/workflows/ci.yml`) runs on `ubuntu-latest`.
2. Native Linux Clang with `llvm-cov` or GCC with `gcov` / `lcov` links `libclang_rt.profile` without issues.
3. Once upstream CI executes, coverage artifacts (`lcov.info`) will be published to the release repository.
