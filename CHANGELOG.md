# Changelog

All notable changes to the Battery Guardian project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0-rc1] - 2026-09-07 (Release Candidate)

### Added
- **Platform API Audit & Telemetry Adapter Layer** (`core/telemetry_adapter.h/.c`):
  - Formal audit of all `furi_hal_power.h` and `power.h` functions against SDK headers (Target 7, API 87.1).
  - Classified dependencies into `SUPPORTED`, `SUPPORTED WITH LIMITATIONS`, and `UNAVAILABLE` categories.
  - Strict SI unit normalization: voltage in Volts, current in Amperes, temperature in Celsius, SOC in %, timestamps in monotonic milliseconds.
  - Explicit `TELEMETRY_CAP_*` capability bitmask and `TelemetryState` operational state (Normal, Partial, Fault, Unavailable).
  - Swappable backend architecture enabling mock injection for test isolation.
- **Charger HAL Capabilities Bitmask Architecture** (`phase2/charger_hal.h`, `core/charger_hal.c`):
  - `ChargerCapability` bitmask (`CHARGER_CAP_CHARGE_CONTROL`, `CHARGER_CAP_CHARGE_STATUS`, `CHARGER_CAP_BATTERY_VOLTAGE`, `CHARGER_CAP_BATTERY_CURRENT`, `CHARGER_CAP_BATTERY_TEMP`, `CHARGER_CAP_BATTERY_SOC`, `CHARGER_CAP_GAUGE_STATUS`).
  - High-level `charger_hal_get_capabilities()`, `charger_hal_is_available()`, `charger_hal_request_charge_enable()`, `charger_hal_request_charge_disable()` wrappers.
  - Production binary remains strictly **PASSIVE and FAIL-CLOSED** (zero PMIC register writes; `CHARGER_CAP_CHARGE_CONTROL` not advertised).
- **Phase 4 Platform Adapter Test Suite** (`tests/platform/test_platform_adapter.c`):
  - 8 new integration tests covering: SI unit normalization, capability bitmasks, custom backend injection, sensor failure state transitions, charger capability queries, passive production fail-closed contract, missing capability graceful handling, and deterministic multi-phase scenario (Boot → USB → Charging → Thermal Excursion → Lockout → Recovery → USB Unplug).
- **Reconciled & Separated Test Reporting**:
  - Phase 3 Hardening (6) and Permanent Regressions (5) are now reported as distinct named suites.
  - Property fuzz transitions (100,000) reported separately from unit and integration test counts.
  - Grand total: 111 unit/integration tests + 100,000 property fuzz transitions, all PASS.
- **UI Production State Honesty Updates**:
  - Dashboard header shows `REALITY (WAIT)` on boot, `REALITY [FAULT]` on gauge failure.
  - Diagnostics view now uses real `gauge_ok` flag (not just VALID_HEALTH), shows `Ctrl: Passive (Safe)`.
- **Hardware Validation Manual** (`HARDWARE_VALIDATION.md`):
  - Full physical bench test protocol with required instruments, wiring, and step-by-step procedures.
  - 6 detailed test protocols: B-01 Telemetry Accuracy, B-02 Charge Suppression Electrical, B-03 Thermal Excursion, B-04 USB Bounce & Disconnect, B-05 I2C Fault Injection, B-06 Degraded Battery Identification.
  - Status: `PHYSICAL HARDWARE VALIDATION: NOT PERFORMED`.
- **Release Artifacts**:
  - `dist/battery_guardian.fap` (SHA256: `961FA1022859BE8CC51C12331C0CA793FC282EE5A60CBB3E12A3B67A921E3583`)
  - `dist/battery_guardian-v1.0.0-rc1.fap` (identical binary, versioned copy)

### Changed
- `ARCHITECTURE.md` completely rewritten to document platform API audit table, Telemetry Adapter boundary, Charger HAL capability contract, full concurrency lock hierarchy, adaptive sampling rates, and storage quota behavior.
- `core/telemetry.c` now delegates all hardware polling through `telemetry_adapter_read_sample()` rather than calling `furi_hal_power_*` directly.

---

## [1.0.0] - 2026-09-06

### Added
- **Storage & Persistence Hardening**:
  - Replaced unsafe `(uint16_t)` casts and unbounded `malloc` in journal recovery with atomic `storage_file_truncate()` and 512-byte streaming chunk fallback.
  - Enforced `JOURNAL_MAX_FILE_SIZE` quota (512 KB) across writes and flushes with rate-limited warning logs.
  - Added strict `JournalStatus` error reporting and format version mismatch protection (`JournalStatusErrorVersionMismatch`).
- **Numerical & Lifetime Hardening**:
  - Implemented 64-bit monotonic timestamp tracking across 32-bit `furi_get_tick()` rollovers (~49.7 days).
  - Added defensive divide-by-zero guards in capacity estimator, linear regression, and health snapshot calculations.
  - Fixed storage lifetime in `app.c`: `RECORD_STORAGE` is maintained open for the complete lifetime of `BatteryGuardianApp`.
  - Removed obsolete prototype files (`core/confidence.h`, `phase2/*.c_disabled`).
- **Diagnostic Observability Layer**:
  - Added `core/diagnostics.h` and `core/diagnostics.c` providing thread-safe diagnostic counters (samples read/invalid, sessions completed, journal writes/drops, policy decisions, lockouts) and unified `BG_LOG_*` macros.
  - Integrated live diagnostic counters and charge policy states into `DiagnosticsView`.
- **UI & Navigation Enhancements**:
  - Fixed Left button on Dashboard view to correctly navigate to `BatteryGuardianSceneHealth`.
- **Expanded Test Matrix & Permanent Regressions**:
  - Created `tests/hardening/test_journal_hardening.c` (>64KB truncation, quota enforcement, format version rejection).
  - Created `tests/hardening/test_hostile_telemetry.c` (divide-by-zero guards, extreme sensor values, 32-bit tick rollover).
  - Created permanent regression suite `tests/regression/test_regressions.c` covering `REG_01` through `REG_05`.
  - Expanded test suite to 103 unit/integration tests and 100,000 property fuzz transitions (100% PASS).
- **Comprehensive Documentation Suite**:
  - Added `README.md`, `ARCHITECTURE.md`, `BUILD.md`, `TESTING.md`, `DATA_FORMAT.md`, `SAFETY.md`, `DEVELOPMENT.md`, `CHANGELOG.md`, and `LICENSE`.

---

## [0.2.0] - Phase 2B (Charge Policy & Safety Engine)

### Added
- Charge policy engine supporting `BALANCED` (80%), `LIFESPAN` (60%), `FULL` (100%), and `CUSTOM` modes.
- Multi-state safety state machine with hysteresis guards and fail-safe transitions (USB disconnect, voltage out-of-bounds, temperature fault, gauge error).
- Unified `ChargerHalInterface` with passive logging stubs in production (`FURI_LOG_W`) and mock controls in host test mode.
- 100,000-iteration randomized property fuzzing suite verifying zero safety violations.

---

## [0.1.5] - Phase 2A (Intelligence & Degradation Modeling)

### Added
- Coulombic capacity estimator with qualification checks ($\Delta\text{SOC} \ge 15\%$, duration, sample count).
- Median-based outlier filter rejecting anomalous sessions ($\pm 15\%$).
- Multi-factor confidence scoring engine (`High`, `Medium`, `Low`, `Insufficient`).
- Long-term degradation tracking using ordinary least-squares linear regression.
- 10 hardware-less synthetic simulation datasets reproducing realistic battery degradation, temperature variance, and sensor glitches.

---

## [0.1.0] - Phase 1 (Core Pipeline & Journaling)

### Added
- Core telemetry sampling engine with adaptive rates (Idle, Active, Charging, Discharging, Anomaly).
- Crash-resilient append-only binary journal with per-record CRC32 verification.
- Continuous session state machine tracking charging and discharging sessions.
- Semantic event generator with configurable debouncing.
- Multi-scene Flipper UI (Dashboard, History, Sessions, Diagnostics, Menu).
