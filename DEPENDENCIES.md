# Battery Guardian — Dependency & Supply-Chain Security Audit

This document audits all direct, indirect, build-time, and runtime dependencies for Battery Guardian (`v1.0.0-rc1`).

---

## 1. Supply-Chain Design Philosophy

Embedded applications running on microcontroller hardware (STM32WB55) operate under strict memory, flash, and stability constraints. To eliminate supply-chain attack vectors and minimize footprint:
- **Zero Third-Party Runtime C Libraries:** Battery Guardian contains **zero external third-party C library dependencies** (no external JSON parsers, no network clients, no bloated utility libraries).
- **Pure C99 & Platform Standard APIs:** All runtime functionality is implemented strictly using ISO C99 standard library facilities and public Flipper SDK core services (`furi`, `furi_hal_power`, `storage`, `gui`).

---

## 2. Dependency Inventory Matrix

| Dependency | Scope | Version / Source | License | Purpose | Supply-Chain Risk & Mitigation |
|---|---|---|---|---|---|
| **Furi OS Core (`furi`)** | Runtime (Target) | API 87.1 / Flipper Firmware `dev` | GPL-3.0 / BSD-3 | FreeRTOS abstraction, threading, mutexes, timers, message queues. | **LOW**: Official Flipper operating system kernel. Bundled into device firmware. |
| **Flipper Power HAL (`furi_hal_power`)** | Runtime (Target) | API 87.1 | GPL-3.0 / BSD-3 | Access to battery voltage, current, temperature, and fuel gauge status. | **LOW**: Official hardware abstraction layer. Guarded behind `telemetry_adapter` boundary. |
| **Flipper Storage Subsystem (`storage`)** | Runtime (Target) | API 87.1 | GPL-3.0 / BSD-3 | FAT32/exFAT filesystem access for append-only binary journal. | **LOW**: Official storage service. Protected by CRC32 framing and 512 KB quota limit. |
| **Flipper GUI Subsystem (`gui`)** | Runtime (Target) | API 87.1 | GPL-3.0 / BSD-3 | View dispatcher, scene manager, and canvas 128x64 display rendering. | **LOW**: Official graphical framework. Thread-safe snapshot decoupling ensures zero UI lockups. |
| **C Standard Library (`libc`)** | Runtime (Target) | Newlib (ARM embedded) | Various Permissive (BSD/MIT) | Math (`sqrtf`, `fabsf`), memory (`memset`, `memcpy`, `malloc`), strings (`snprintf`). | **LOW**: Industry standard embedded libc provided by official ARM GCC toolchain. |
| **ufbt (Micro Flipper Build Tool)** | Build-Time | Current official (v0.4+) | Apache-2.0 | SCons-based target builder that fetches official Flipper SDK and ARM toolchain. | **LOW**: Official tool maintained by Flipper Devices. Verified with clean zero-warning builds. |
| **Python** | Build-Time & Testing | Python 3.8+ | PSF License | Executes test runner (`tests/run_tests.py`) and validation script (`scripts/validate.py`). | **LOW**: Standard interpreter used only on developer workstation; not included in target binary. |
| **Host C Compiler** | Testing (Host) | Zig 0.13.0 / Clang / GCC | MIT / Apache-2.0 / GPL-3.0 | Cross-platform C99 compilation for host unit test suite and property fuzzer. | **LOW**: Local compilation only; zero code introduced into target FAP. |

---

## 3. Dependency Vulnerability Assessment

- **Known CVEs:** None. All platform libraries are tracked against official Flipper Zero firmware releases.
- **External Network Access:** Zero runtime network connections, telemetry beacons, or cloud endpoints. The FAP is 100% offline.
- **Transitive Dependencies:** Zero unvetted transitive libraries.
