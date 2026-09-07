# Battery Guardian — External Application vs. Firmware Boundary Analysis

**Architectural Target:** Flipper Zero External Application (`.fap`)  
**Firmware Compatibility:** Official Release 0.101.x+, Release Candidate, and compatible forks  
**API Level:** 87.1 (Target 7, STM32WB55)

---

## 1. Context & Architectural Strategy

Official Flipper Zero developer guidelines specify:
> *"If an idea can be implemented as an external application, it is usually better to implement it as such and publish it in the App Catalog."*

Battery Guardian adheres strictly to this principle. It is architected to operate entirely as a user-space external application (`FlipperAppType.EXTERNAL`) rather than requiring custom firmware forks, patched operating system binaries, or privileged kernel modifications.

---

## 2. Platform API Dependency Audit

Every platform service used by Battery Guardian has been audited against the official Flipper SDK:

| Platform API / Service | Header | Function Used | Current Status | Firmware Modification Needed? |
|---|---|---|---|:---:|
| **Furi Core OS** | `furi.h` | Threads, Mutexes, Timers, Ticks | **SUPPORTED** (Standard) | NO |
| **Power Subsystem** | `furi_hal_power.h` | `furi_hal_power_get_battery_voltage()`<br/>`furi_hal_power_get_battery_current()`<br/>`furi_hal_power_get_battery_temperature()`<br/>`furi_hal_power_get_pct()`<br/>`furi_hal_power_gauge_is_ok()`<br/>`furi_hal_power_is_charging()` | **SUPPORTED** (Public HAL) | NO |
| **Charge Suppression** | `furi_hal_power.h` | `furi_hal_power_suppress_charge_enter()`<br/>`furi_hal_power_suppress_charge_exit()` | **SUPPORTED WITH LIMITATIONS** (Simulated / Passive in RC1) | NO |
| **Storage Subsystem** | `storage/storage.h` | File read, write, seek, truncate, FS info | **SUPPORTED** (Standard Record) | NO |
| **GUI & View Dispatcher** | `gui/gui.h`<br/>`gui/view_dispatcher.h`<br/>`gui/scene_manager.h` | Standard Scene/View stack, canvas drawing | **SUPPORTED** (Standard UI) | NO |

---

## 3. Detailed Boundary Analysis: Features & System Requirements

### 3.1 Telemetry Observation
- **Current Status:** Fully operational as an external FAP.
- **Alternative:** Running as a kernel daemon inside firmware.
- **Reason:** Reading fuel gauge and ADC registers via `furi_hal_power_*` is already a public non-blocking API available to any external application.
- **Blocker:** **NONE**.

### 3.2 Coulombic Capacity Estimation & Health Modeling
- **Current Status:** Fully operational as an external FAP.
- **Alternative:** Baked into the OS power daemon (`power.c`).
- **Reason:** Computational logic (numerical integration, outlier filtering, linear regression) requires no privileged hardware access and executes in a background worker thread allocated by the FAP.
- **Blocker:** **NONE**.

### 3.3 Persistent Journal Storage
- **Current Status:** Fully operational as an external FAP using `/ext/apps_data/battery_guardian/` on the SD card.
- **Alternative:** Writing to internal SPI flash.
- **Reason:** Flipper internal flash is severely constrained (saving space for firmware and settings). External SD card storage (`RECORD_STORAGE`) offers megabytes of storage space and standard FAT32 portability.
- **Blocker:** **NONE**.

### 3.4 Continuous Background Sampling Across OS Apps
- **Current Status:** When the user exits the Battery Guardian FAP, sampling pauses.
- **Alternative:** Running a continuous background daemon (`FuriThread`) while other applications (NFC, Sub-GHz) are active.
- **Reason:** Furi OS architecture allows only one primary external graphical application at a time. Background worker threads can exist, but running persistent background services indefinitely without user awareness is discouraged by community app guidelines to preserve RAM and battery life.
- **Path Forward:** Battery Guardian handles start/stop cleanly: when reopened, it parses the journal, calculates elapsed time, and maintains session continuity without data corruption.
- **Blocker:** **OPTIONAL ARCHITECTURAL DISCUSSION** (Documented in `QUESTIONS_FOR_REVIEW.md`).

### 3.5 Active Charge Suppression Control
- **Current Status:** Currently configured as passive fail-closed in production (`v1.0.0-rc1`).
- **Alternative:** Direct BQ25896 I2C register writes.
- **Reason:** `furi_hal_power_suppress_charge_enter()` and `furi_hal_power_suppress_charge_exit()` exist in official firmware headers (`furi_hal_power.h`). However, because active control alters device power intake, it remains passive in RC1 until physical bench validation confirms hardware stability.
- **Blocker:** **PHYSICAL BENCH VALIDATION ONLY (No firmware modifications required)**.

---

## 4. Conclusion

Battery Guardian has **zero dependencies on custom firmware forks**. 100% of its observation, modeling, journaling, and user interface features run within the official Flipper Zero external application sandbox.
