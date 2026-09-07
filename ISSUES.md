# Battery Guardian — Project Issue Inventory & Triage

This document tracks all known issues, technical debts, design constraints, and future enhancement proposals. Items are categorized according to their operational resolution state.

---

## 1. Blocked by Hardware Validation

| Issue ID | Subsystem | Description | Resolution Gate |
|---|---|---|---|
| **HW-01** | `core/charger_hal.c` | **Active Charge Control Disabled in Production:** Charge suppression returns `false` unconditionally to prevent untested I2C register writes to PMIC. | Blocked until Bench Protocols B-01, B-04, B-05, B-06, B-03, and B-02 pass. |
| **HW-02** | `phase2/charge_policy.c` | **Physical Thermal Cutoff Verification:** 45.0°C software emergency lockout must be tested on physical bench with calibrated thermocouple and thermal chamber. | Blocked until Bench Protocol B-03 passes. |
| **HW-03** | `storage/journal.c` | **microSD Physical Wear & Latency Bench:** Journal write latency across budget vs high-speed microSD cards must be benchmarked on hardware. | Blocked until Bench Protocol B-04 passes. |

---

## 2. Needs Maintainer Input

| Issue ID | Subsystem | Description | Action Item |
|---|---|---|---|
| **MAINT-01** | `core/telemetry.c` | **Background Sampling Daemon Pattern:** Determine preferred Furi OS pattern for long-lived background telemetry sampling across app transitions. | Documented in `QUESTIONS_FOR_REVIEW.md` (Q3). |
| **MAINT-02** | `core/charger_hal.c` | **Public Charge Suppression API Stability:** Confirm future roadmap for `furi_hal_power_suppress_charge_*`. | Documented in `QUESTIONS_FOR_REVIEW.md` (Q1). |
| **MAINT-03** | `storage/journal.c` | **Storage Path Canonical Convention:** Confirm `/ext/apps_data/battery_guardian/` is the preferred storage path for catalog FAPs. | Documented in `QUESTIONS_FOR_REVIEW.md` (Q4). |

---

## 3. Known Limitations

| Issue ID | Subsystem | Description | Impact |
|---|---|---|---|
| **LIMIT-01** | `core/session.c` | **Discharge Session Qualification Threshold:** Sessions with $\Delta\text{SOC} < 15\%$ are disqualified from capacity learning. | Users operating in shallow 5% cycles will see `REALITY (WAIT)` until a deeper discharge occurs. |
| **LIMIT-02** | `storage/journal.c` | **512 KB Storage Quota Ceiling:** Once `journal.bin` reaches 524,288 bytes, raw telemetry sample logging drops until manual user cleanup. | Checkpoints are preserved, but high-resolution 10s samples cease. |
| **LIMIT-03** | `gui/views/events.c` | **Uptime Clock Display:** Event timestamps display monotonic uptime elapsed (`HH:MM`) rather than RTC wall-clock time. | Avoids RTC sync issues, but does not display calendar hour. |

---

## 4. Future Enhancements (Post-v1.0)

| Issue ID | Subsystem | Description | Target Milestone |
|---|---|---|---|
| **FEAT-01** | `storage/journal.c` | **Automated Journal Archival & Compaction:** When quota is reached, compress or rotate `journal.bin` to `journal.old.bin`. | v1.1.0 |
| **FEAT-02** | `gui/views/health.c` | **Interactive Battery Calibration Wizard:** Step-by-step UI guiding user through a full calibration discharge to generate High-confidence baseline. | v1.1.0 |
| **FEAT-03** | `core/session.c` | **Configurable Session Qualification Threshold:** Allow advanced users to lower the 15% threshold in custom settings. | v1.1.0 |

---

## 5. Resolved Issues

| Issue ID | Subsystem | Description | Resolution Phase |
|---|---|---|---|
| **REG-01** | `core/session.c` | `SESSION_QUALITY_NO_TEMP_EXCURSION` was not initialized at session start. | Fixed in Phase 2B (Verified by `REG_01`). |
| **REG-02** | `storage/journal.c` | Journal recovery used implicit 16-bit offset cast, silently truncating files >64KB. | Fixed in Phase 3 (Verified by `REG_02`). |
| **REG-03** | `phase2/battery_health.c` | Division-by-zero when reference capacity was 0.0f produced NaN. | Fixed in Phase 3 (Verified by `REG_03`). |
| **REG-04** | `app.c` | `RECORD_STORAGE` handle was opened/closed repeatedly rather than held for app lifecycle. | Fixed in Phase 3 (Verified by `REG_04`). |
| **REG-05** | `phase2/charge_policy.c` | USB disconnect event did not immediately clear charge suppression flag. | Fixed in Phase 3 (Verified by `REG_05`). |
| **SEC-01** | `storage/journal.c` | Recursive mutex deadlock on `journal_free()`. | Fixed in Phase 5 (Extracted `journal_flush_locked()`). |
| **SEC-02** | `storage/journal.c` | Payload length bounds check missing on `journal_write_record()`. | Fixed in Phase 5 (Added `length > 1024` check). |
| **SEC-05** | `core/diagnostics.c` | Diagnostic counters wrapped around to 0 at `UINT32_MAX`. | Fixed in Phase 5 (Added `saturating_inc()`). |
| **SEC-06** | `phase2/charge_policy.c` | Custom policy parameters lacked defensive bounds clamping. | Fixed in Phase 5 (Clamped to 50–100%, 1–20%). |
| **BUILD-01**| `application.fam` | `phase2/*.c` wildcard unintentionally included `test_phase2.c` in target build. | Fixed in Phase 5 (Explicit source list). |
