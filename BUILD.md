# Building Battery Guardian

This guide covers building the host test harness and compiling the Flipper Application Package (`.fap`) for the Flipper Zero.

---

## 1. Prerequisites

### For Target Device Build (`.fap`)
- **Python 3.8+**
- **ufbt (Micro Flipper Build Tool)**:
  Install via pip:
  ```bash
  pip install --upgrade ufbt
  ```
  Ensure `ufbt` has downloaded the latest SDK:
  ```bash
  ufbt update
  ```
- **Target Platform:** Flipper Zero STM32WB55 (Target 7, API 87.1)

### For Host Simulation & Unit Testing
A C99 compiler is required to run the 118 unit tests and 100,000 property fuzz transitions locally.
Install using the exact command for your platform:

- **Windows (PowerShell):**
  ```powershell
  winget install -e --id zig.zig
  ```
  *(Or install LLVM Clang: `winget install -e --id LLVM.LLVM`)*

- **Ubuntu / Debian Linux:**
  ```bash
  sudo apt-get update
  sudo apt-get install -y build-essential clang
  ```

- **macOS (Homebrew):**
  ```bash
  brew install zig
  ```
  *(Or install Command Line Tools: `xcode-select --install`)*

The test runner `tests/run_tests.py` automatically detects `zig`, `clang`, or `gcc` on `PATH`.
You can also override the compiler by setting `CC`:
```bash
CC=clang python tests/run_tests.py
```

---

## 2. Compiling the FAP (ARM Cortex-M4)

To build the application package for Flipper Zero from a clean state:

```bash
# 1. Navigate to the battery_guardian root directory
cd battery_guardian

# 2. Clean previous build artifacts
ufbt clean

# 3. Compile the application
ufbt
```

### Build Output & Verification
Upon successful compilation, the resulting binary is placed in:
- `dist/battery_guardian.fap` — Release FAP binary for SD card installation
- `dist/debug/battery_guardian_d.elf` — Debug ELF binary with full DWARF symbols

Verify the artifact exists and check size:
```bash
# Windows PowerShell
Get-Item dist/battery_guardian.fap | Select-Object Length, LastWriteTime
Get-FileHash dist/battery_guardian.fap -Algorithm SHA256

# Linux / macOS
ls -la dist/battery_guardian.fap
sha256sum dist/battery_guardian.fap
```

### Deploying to Connected Flipper
If your Flipper Zero is connected via USB:
```bash
# Build, install to /ext/apps/Tools/battery_guardian.fap, and launch
ufbt launch
```

---

## 3. Running Host Test Suites

The host test suite compiles and executes natively without requiring physical hardware:

```bash
# Run with auto-detected compiler
python tests/run_tests.py

# Or specify a custom compiler via CC
CC=clang python tests/run_tests.py
```

### Verified Test Matrix (118 Tests + 100,000 Fuzz Iterations)
The runner compiles and runs:
1. **Phase 1 (Core Pipeline & Journal)**: 23 tests (Telemetry, Journal, Session State Machine, Events, Integration Pipeline)
2. **Phase 2A (Health & Capacity Intelligence)**: 39 tests (Estimator, Outlier rejection, Synthetic datasets, Adversarial sessions, Confidence, Stale model, Scale)
3. **Simulation Datasets**: 10 datasets (Stable, Degradation, Outlier, Partial, Interrupted, Determinism, Temperature, Gauge mismatch, Pathological, 10k stress)
4. **Phase 2B (Charge Policy & Safety Machine)**: 20 tests (Policies, Hysteresis, USB disconnect, Fault lockouts, Deterministic replay, Bounded memory)
5. **Phase 3 (Storage & Numerical Hardening)**: 6 tests (>64KB journal truncation, 512KB quota enforcement, format version rejection, divide-by-zero guards, hostile values, 32-bit tick rollover)
6. **Permanent Regression Suite (REG_01–REG_05)**: 5 tests (Historical bug protection)
7. **Phase 4 (Platform Adapter & Hardware Abstraction)**: 8 tests (SI normalization, capabilities, fail-closed contract, deterministic scenario)
8. **Phase 5 (Security Audit & Adversarial Persistence)**: 7 tests (Mutex deadlock prevention, bounded records, malicious fixtures, bitflip CRC rejection, saturating counters, config clamps, null guards)
9. **Property Fuzz Transitions**: 100,000 randomized state machine transitions verifying zero safety invariant violations.

---

## 4. Clean Build Verification Procedure

To perform an authoritative clean release build from scratch:

```bash
# 1. Clean host build artifacts
python -c "import shutil, pathlib; shutil.rmtree('build', ignore_errors=True)"

# 2. Run complete test suite
python tests/run_tests.py

# 3. Clean target build artifacts
ufbt clean

# 4. Build target ARM FAP
ufbt

# 5. Generate versioned release candidate copy
python -c "import shutil; shutil.copyfile('dist/battery_guardian.fap', 'dist/battery_guardian-v1.0.0-rc1.fap')"
```

---

## 5. Troubleshooting

### Compiler not found
If `python tests/run_tests.py` reports `Error: Suitable C compiler not found`, ensure `zig`, `clang`, or `gcc` is in your `PATH`:
- Windows: Install Zig via `winget install zig.zig` or Scoop `scoop install zig`
- Linux: `sudo apt-get install clang` or `sudo apt-get install gcc`
- macOS: `brew install zig` or Xcode Command Line Tools (`xcode-select --install`)

### SCons / ufbt SDK version mismatch
If `ufbt` fails with API version errors:
```bash
ufbt update --channel=release
ufbt status
```
Ensure target is Target 7, API 87.1.
