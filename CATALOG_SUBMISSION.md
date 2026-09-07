# Battery Guardian — Flipper Apps Catalog Submission Package

> [!NOTE]
> **Status: DRAFT PREPARATION ONLY**  
> This document prepares the metadata, copy, and structural declarations required for eventual submission to the official Flipper Application Catalog (`flipper-application-catalog`). It does **not** imply current submission or approval.

---

## 1. Application Metadata

- **Application Name:** Battery Guardian
- **Application ID (`appid`):** `battery_guardian`
- **Category:** Tools
- **Version:** `1.0.0` (Release Candidate `v1.0.0-rc1`)
- **Author:** Battery Guardian Contributors
- **License:** MIT License
- **Repository URL:** `https://github.com/flipperdevices` *(Staging / Community Fork)*
- **Target Architecture:** ARM Cortex-M4 (STM32WB55, Target 7)
- **Minimum Firmware API:** `87.1` (Firmware `0.101.0` or newer)
- **MicroSD Card Required:** YES (for persistent telemetry journal)

---

## 2. Public Application Copy

### 2.1 Short Description (Catalog Summary - max 100 chars)
> *Battery health intelligence, session capacity estimation, degradation tracking, and safety monitor.*

### 2.2 Long Description (Catalog Markdown)
```markdown
Battery Guardian is an advanced battery intelligence, telemetry observation, and charge safety monitoring application for the Flipper Zero.

Instead of relying solely on factory lookup tables that report inaccurate percentages on aging or degraded cells, Battery Guardian observes real-world battery behavior:
- Integrates actual Coulombic energy (current over time) during qualifying discharge sessions.
- Filters statistical outliers using median filtering over session history.
- Computes multi-factor confidence ratings (High, Medium, Low, Insufficient) before presenting capacity estimates.
- Derives long-term degradation trajectories using linear regression.
- Records telemetry to a crash-resilient, CRC32-verified binary journal on the SD card (capped at 512 KB).
- Evaluates multi-state charge policies (Balanced 80%, Lifespan 60%, Full 100%) with hysteresis and thermal lockout guards.

Includes 5 detailed views:
- Dashboard: Real-time SI telemetry, battery state, and active power mode.
- Health & Degradation: True observed capacity, nominal comparison, and wear trend.
- History: Visual historical discharge graph with median filtering.
- Sessions: Chronological log of qualified discharge sessions and energy metrics.
- Diagnostics: Low-level sensor validity flags, crash counters, and safety FSM status.
```

### 2.3 Feature Summary
- **Physical SI Telemetry:** True Volts, Amperes, Celsius, and State of Charge.
- **Dynamic Session Learning:** Minimum 15% discharge drop qualification ensures mathematical accuracy.
- **Crash-Resilient Journal:** CRC32 protected append-only binary log with self-healing recovery.
- **Multi-Factor Confidence:** Transparently indicates when more data is needed (`REALITY (WAIT)`).
- **Formal Safety State Machine:** Full hysteresis protection against rapid PMIC cycling.

### 2.4 Safety Disclosure & Hardware Boundary
> [!WARNING]
> **Safety Notice: Passive Monitoring in v1.0.0-rc1**  
> In this release, Battery Guardian operates in a **strictly passive, fail-closed configuration**. Charge policies and safety lockouts are mathematically evaluated and logged, but **zero physical register writes** are made to the charging hardware. Physical hardware validation must be completed before active charge suppression is enabled.

### 2.5 Known Limitations
- Requires at least 3 qualifying discharge sessions ($\ge 15\%$ drop) before displaying an observed health estimate.
- Storage journal is capped at 512 KB to preserve SD card space; raw sample logging pauses once quota is reached.
- Telemetry sampling pauses when exiting the application back to Flipper desktop.

---

## 3. Manifest Declaration (`application.fam`)

```python
App(
    appid="battery_guardian",
    name="Battery Guardian",
    apptype=FlipperAppType.EXTERNAL,
    entry_point="battery_guardian_app",
    cdefines=["APP_BATTERY_GUARDIAN"],
    requires=[
        "gui",
        "cli",
        "power",
        "storage",
    ],
    stack_size=4 * 1024,
    order=20,
    fap_category="Tools",
    fap_author="Battery Guardian Contributors",
    fap_weburl="https://github.com/flipperdevices",
    fap_version="1.0",
    fap_description="Battery health intelligence, degradation tracking, and charge safety guardian",
    sources=[
        "app.c",
        "core/*.c",
        "storage/*.c",
        "model/*.c",
        "phase2/battery_health.c",
        "phase2/charge_policy.c",
        "phase2/confidence.c",
        "phase2/degradation.c",
        "phase2/estimator.c",
        "gui/**/*.c",
    ],
)
```

---

## 4. Required Catalog Media Assets

| Asset Name | Format | Dimensions | Purpose |
|---|---|---|---|
| `icon.png` | 1-bit monochrome PNG | 10x10 px | Main menu list icon |
| `screenshot_dashboard.png` | 1-bit monochrome PNG | 128x64 px | Dashboard live telemetry view |
| `screenshot_health.png` | 1-bit monochrome PNG | 128x64 px | Health and capacity estimation view |
| `screenshot_history.png` | 1-bit monochrome PNG | 128x64 px | Historical discharge graph |
| `screenshot_diagnostics.png` | 1-bit monochrome PNG | 128x64 px | Diagnostics and safety state view |

For capture specifications and simulation guidelines, see [SCREENSHOT_PLAN.md](SCREENSHOT_PLAN.md).

---

## 5. Support & Security Information

- **Issue Tracker:** Submit via GitHub Issue Tracker using the bug report template.
- **Security Vulnerabilities:** Follow responsible disclosure policy in [SECURITY.md](SECURITY.md).
