# Battery Guardian — Current Project Status

```
================================================================================
SOFTWARE VALIDATION:      COMPLETE (118/118 Tests PASS, 100,000 Property Fuzz PASS)
EXTERNAL APPLICATION:     READY (100% External FAP; Zero Firmware Modifications)
RELEASE CANDIDATE:        v1.0.0-rc1
PUBLIC REMOTE:            NOT CONFIGURED (Local Repository Clean & Tagged)
PHYSICAL HARDWARE:        NOT VALIDATED
ACTIVE CHARGE CONTROL:    DISABLED / FAIL-CLOSED (Zero PMIC Register Writes)
CATALOG:                  PREPARED (Conforms to Current Official Catalog Schema)
MAINTAINER REVIEW:        PENDING
================================================================================
```

---

## 1. Verified Software Status

The software architecture of Battery Guardian is **feature-frozen** and validated:
- **Phase 1 (Core Pipeline & Journal):** 23 / 23 PASS
- **Phase 2A (Health & Capacity Intelligence):** 39 / 39 PASS
- **Simulation Datasets (Hardware-Less):** 10 / 10 PASS
- **Phase 2B (Charge Policy & Safety FSM):** 20 / 20 PASS
- **Phase 3 (Storage & Numerical Hardening):** 6 / 6 PASS
- **Permanent Regressions (REG_01 - REG_05):** 5 / 5 PASS
- **Phase 4 (Platform Adapter & Hardware HAL):** 8 / 8 PASS
- **Phase 5 (Security Audit & Adversarial Persistence):** 7 / 7 PASS
- **Total Unit & Integration Tests:** **118 / 118 PASS**
- **Property Fuzz Transitions:** **100,000 / 100,000 PASS** (0 Invariant Violations)
- **Target ARM FAP Build:** **CLEAN** (0 warnings, Target 7, API 87.1)
- **Artifact:** `dist/battery_guardian-v1.0.0-rc1.fap` (51,656 bytes)

---

## 2. Hardware Verification Boundary

> [!WARNING]
> **Physical Hardware Validation: NOT PERFORMED**  
> All charge policies and safety transitions have been proven exclusively in host software simulation and randomized fuzzing. In production firmware builds, the Charger HAL does not advertise charge control capabilities and issues **zero register writes** to the BQ25896 PMIC.

Physical validation protocols (B-01 through B-06) are documented in [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md). The hardware evidence directory [hardware/](hardware/) is staged and ready for real bench data.

---

## 3. Catalog Submission Readiness

- **Manifest:** [manifest.yml](manifest.yml) validated against official catalog specifications.
- **Icon:** [icon.png](icon.png) (10x10, 1-bit, black & white).
- **Screenshots:** [screenshots/](screenshots/) (512x256, exact 2-color qFlipper palette `(254,138,44)` / `(0,0,0)`).
- **Description & Changelog:** [docs/description.md](docs/description.md) and [docs/changelog.md](docs/changelog.md) adhering to catalog markdown restrictions.

---

## 4. Known Limitations

1. **Passive Charger Control:** Active charge suppression cannot halt physical current in v1.0.0-rc1; policy evaluations are simulated and logged only.
2. **Session Qualification:** Requires at least 3 qualifying discharge sessions of $\Delta\text{SOC} \ge 15\%$ before calculating observed capacity.
3. **Storage Quota:** Telemetry journal is capped at 512 KB in `/ext/apps_data/battery_guardian/`.
4. **App Lifetime:** Background sampling pauses when exiting the FAP back to the Flipper desktop.

---

## 5. Next Milestone

Feature development is **FROZEN**. The next milestone is **Physical Hardware Bench Validation + External Maintainer Review**.
