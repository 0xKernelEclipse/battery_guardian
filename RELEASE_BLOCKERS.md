# Battery Guardian — Release Blockers & GA Criteria

This document tracks all formal release blockers separating the current Release Candidate (`v1.0.0-rc1`) from a General Availability (`v1.0.0 GA`) release.

---

## 1. Release Blocker Classification

Release blockers are strictly divided into four categories:
1. **SOFTWARE BLOCKERS:** Algorithmic, computational, memory, or concurrency bugs.
2. **HARDWARE BLOCKERS:** Missing physical bench measurements, silicon verification, or thermal validation.
3. **ECOSYSTEM BLOCKERS:** Manifest compliance, catalog schema, media assets, or platform compatibility.
4. **MAINTAINER-REVIEW BLOCKERS:** Unresolved maintainer feedback, API stability questions, or repository staging.

---

## 2. Current Blocker Matrix

| Blocker ID | Category | Description | Current Status | Resolving Action / Gate |
|---|---|---|:---:|---|
| **BLK-SW-01** | Software | Host unit test failures or test regressions | **RESOLVED** | 118 / 118 tests PASS; 5 regression guards verified. |
| **BLK-SW-02** | Software | Property fuzzing invariant violations | **RESOLVED** | 100,000 / 100,000 transitions verified with 0 violations. |
| **BLK-SW-03** | Software | ARM Cortex-M4 FAP target build | **RESOLVED** | Compiles clean with 0 warnings on SDK 1.4.3 (Target 7, API 87.1). |
| **BLK-HW-01** | Hardware | Baseline Telemetry Accuracy Validation (B-01) | **OPEN** | Physical bench testing with 6.5-digit DMM and DC supply. |
| **BLK-HW-02** | Hardware | USB Contact Bounce & Storage Integrity (B-04) | **OPEN** | Physical mechanical USB-C disconnect cycling on real SD card. |
| **BLK-HW-03** | Hardware | Fuel Gauge I2C Bus Fault Recovery (B-05) | **OPEN** | Physical SDA pull-down pulse test on Flipper test points. |
| **BLK-HW-04** | Hardware | Degraded Battery Pack Identification (B-06) | **OPEN** | Multi-session discharge test on high-impedance cell simulator. |
| **BLK-HW-05** | Hardware | Thermal Excursion Safety Lockout (B-03) | **OPEN** | Controlled thermal chamber test verifying 45.0°C cutoff. |
| **BLK-HW-06** | Hardware | Active Charge Suppression Electrical Transient (B-02) | **OPEN** | Oscilloscope current clamp capture proving clean step-down. |
| **BLK-ECO-01**| Ecosystem | Catalog manifest & media asset compliance | **RESOLVED** | `manifest.yml`, `icon.png`, `screenshots/` conform to official rules. |
| **BLK-ECO-02**| Ecosystem | Public GitHub Remote Configuration | **OPEN** | Push local repository to public GitHub (`0xKernelEclipse/battery_guardian`). |
| **BLK-REV-01**| Maintainer | Feedback on FAP boundary & power HAL APIs | **OPEN** | Submit proposal to Flipper maintainers and evaluate input. |

---

## 3. General Availability (GA) Gate Criteria

Battery Guardian will transition from `v1.0.0-rc1` to `v1.0.0 GA` **only** after:
1. Public repository is live and catalog manifest is validated.
2. Protocols B-01, B-04, B-05, and B-06 pass on real hardware with documented evidence.
3. Protocol B-03 (thermal cutoff) passes on real hardware.
4. Active charge suppression (B-02) is verified if active control is to be enabled, OR consensus is reached to release v1.0 as a passive observability tool.
5. No unresolved safety or data corruption issues remain.
