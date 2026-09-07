# Battery Guardian Development Guide

## 1. Directory Structure

```
battery_guardian/
├── application.fam          # Flipper package manifest
├── app.c                    # Application lifecycle & view dispatcher loop
├── core/                    # Core ingestion pipeline & hardware boundary
│   ├── telemetry.h/.c       # Telemetry sampling & 64-bit monotonic clock
│   ├── session.h/.c         # Continuous session tracking & Coulomb counting
│   ├── event.h/.c           # Semantic event generation & debouncing
│   ├── diagnostics.h/.c     # BG_LOG macros & diagnostic counters
│   └── charger_hal.c        # Production passive stubs & host mocks
├── storage/                 # Persistence
│   ├── journal.h/.c         # Crash-resilient binary journal & recovery
├── phase2/                  # Intelligence & Safety Engines
│   ├── estimator.h/.c       # Coulombic capacity estimator & outlier filter
│   ├── confidence.h/.c      # Multi-factor confidence engine
│   ├── degradation.h/.c     # Linear regression degradation modeling
│   ├── battery_health.h/.c  # Unified health engine facade
│   ├── charge_policy.h/.c   # Policy engine & safety state machine
│   └── charger_hal.h        # Abstract charger HAL interface
├── model/                   # Data models bridging worker thread & GUI
│   ├── battery_model.h/.c   # Background worker thread & snapshot publisher
│   ├── history_model.h/.c   # Ring buffer for historic telemetry
│   ├── session_model.h/.c   # Historic session buffer
│   └── event_model.h/.c     # Semantic event log buffer
├── gui/                     # Flipper UI subsystem
│   ├── scenes/              # Scene handlers (Dashboard, Health, Diagnostics, etc.)
│   └── views/               # Canvas rendering callbacks & input handlers
└── tests/                   # Native test harness & simulation suite
    ├── telemetry/           # Telemetry unit tests
    ├── journal/             # Journal CRC & recovery tests
    ├── session/             # State machine & metric tests
    ├── event/               # Event generation tests
    ├── phase2/              # Phase 2A intelligence unit tests
    ├── simulation/          # 10 hardware-less simulation datasets
    ├── hardening/           # >64KB truncation, quota, hostile telemetry
    ├── regression/          # Permanent regression suite (REG_01 - REG_05)
    ├── mocks/               # Mock Furi OS, Storage, and Power HAL
    └── run_tests.py         # Python test driver
```

---

## 2. Coding Standards & Invariants

1. **C99 Standard**: Code must compile cleanly under both GCC (ARM target via `ufbt`) and Clang (Host via `zig cc`) with zero warnings (`-Wall -Wextra`).
2. **Deterministic Memory**:
   - Zero dynamic allocations (`malloc`, `calloc`) in steady-state execution loops.
   - All buffers must be statically sized or allocated once during initialization.
3. **Monotonic Time**:
   - Never rely on `furi_get_tick()` directly for arithmetic across sessions without passing through `telemetry_get_monotonic_ms()`.
4. **Defensive Numerical Safety**:
   - Every floating-point division must be guarded with explicit zero or epsilon checks (`fabsf(denominator) < 1e-6f`).
   - Nan and Infinity propagation is strictly forbidden.
5. **Hardware Safety**:
   - Any new hardware interaction must be routed through `ChargerHalInterface`.
   - Never call `furi_hal_power_*` write functions directly in application or model code.

---

## 3. Pull Request Checklist

Before submitting changes, ensure the following checks pass:

- [ ] `python tests/run_tests.py` passes all 103 unit tests.
- [ ] Property fuzzing completes 100,000 transitions with 0 violations.
- [ ] `ufbt clean && ufbt` compiles with zero warnings or errors.
- [ ] No regressions introduced in `tests/regression/test_regressions.c`.
- [ ] All new functions and structs are documented with header comments.
