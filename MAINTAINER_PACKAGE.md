# Maintainer Contact & Review Package

**Application:** Battery Guardian (`battery_guardian`)  
**Target:** Flipper Zero (STM32WB55, Target 7, API 87.1)  
**Package Type:** External FAP (`FlipperAppType.EXTERNAL`)  
**Release Candidate:** `v1.0.0-rc1`  
**Author:** 0xKernelEclipse  
**Repository:** Staged locally (Public remote not yet configured)  
**Verification:** 118 / 118 Unit/Integration Tests PASS, 100,000 Property Fuzz Transitions PASS  
**Physical Bench Validation:** NOT PERFORMED  
**Active Charger Control:** DISABLED / FAIL-CLOSED  

---

## 1. Problem Statement

Standard embedded battery state is reduced to an instantaneous State of Charge percentage (`SOC %`), derived from static factory lookup tables. On aging, cold, or degraded battery packs:
1. Voltage depression under load causes false drop readings.
2. Fuel gauge tables drift, reporting 100% on worn cells that possess only a fraction of nominal capacity.
3. No historical telemetry or degradation slope is tracked.
4. Charging is unmanaged without cycle-preservation options (e.g. 80% or 60% ceilings).

Battery Guardian addresses this by implementing an external battery intelligence engine:
- Continuous physical SI telemetry observation (V, A, °C, SOC).
- Coulombic energy integration over qualifying discharge sessions ($\Delta\text{SOC} \ge 15\%$).
- Median-based outlier filtering over an in-memory 16-session history buffer.
- Multi-factor confidence scoring (`High`, `Medium`, `Low`, `Insufficient`).
- Ordinary Least Squares (OLS) linear regression for wear tracking.
- Crash-resilient append-only binary journal with per-record CRC32 verification (512 KB quota).
- Multi-state charge policy state machine with hysteresis guards and thermal lockouts.

---

## 2. Why an External FAP?

In accordance with Flipper's official contributing guidelines:
> *"If an idea can be implemented as an external application, it is usually better to implement it as such and publish it in the App Catalog."*

Battery Guardian requires **zero firmware modifications**. It runs entirely in user-space via standard public SDK APIs (`furi`, `furi_hal_power`, `storage`, `gui`).

---

## 3. Strict Hardware Safety Boundary

> [!IMPORTANT]
> **Physical Hardware Validation Status: NOT PERFORMED**
> In `v1.0.0-rc1`, the Charger HAL (`core/charger_hal.c`) operates in a strictly **PASSIVE and FAIL-CLOSED** mode:
> - `CHARGER_CAP_CHARGE_CONTROL` is not advertised in production capabilities.
> - Charge suppression calls return `false` unconditionally.
> - **Zero register writes** are performed to the BQ25896 PMIC.
> - The software models and logs policies safely without altering device charging hardware.

---

## 4. Test & Verification Evidence

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
Target ARM Compilation (ufbt, Target 7):       CLEAN (0 warnings)
```

Run one-command local validation:
```bash
python scripts/validate.py
```

---

## 5. Technical Questions for Maintainers

1. **API Stability:** Are `furi_hal_power_suppress_charge_enter()` and `_exit()` planned to remain part of the public HAL contract?
2. **Background Daemons:** Does the ecosystem support a recommended pattern for lightweight background telemetry polling in external FAPs, or should all background polling be restricted to foreground execution?
3. **Storage Canonical Paths:** Is `/ext/apps_data/battery_guardian/` the accepted canonical path for catalog FAP persistent storage?

---

## 6. Catalog Submission Readiness

- Manifest prepared: [manifest.yml](manifest.yml) conforming to current official catalog schema.
- Icon prepared: [icon.png](icon.png) (10x10, 1-bit, black & white).
- Screenshots prepared: [screenshots/](screenshots/) (512x256, exact qFlipper orange/black palette `(254,138,44)` / `(0,0,0)`).
- Description & Changelog: [docs/description.md](docs/description.md) and [docs/changelog.md](docs/changelog.md) using only catalog-allowed markdown.
