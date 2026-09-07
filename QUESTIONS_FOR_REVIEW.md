# Battery Guardian — Technical Questions for Firmware Maintainers & Reviewers

We welcome architectural feedback from the Flipper Zero core firmware maintainers and the embedded development community. To ensure review time is used effectively, we have compiled specific, technically focused questions regarding platform constraints, API evolution, and ecosystem integration.

---

## 1. Platform Power API Stability & Evolution

1. **Charge Suppression Function Stability:**
   - Battery Guardian references `furi_hal_power_suppress_charge_enter()` and `furi_hal_power_suppress_charge_exit()` declared in `furi_hal_power.h`.
   - *Question:* Are these APIs considered part of the stable public external FAP contract across future firmware releases (`0.102+` / `1.0`), or are they slated for deprecation, refactoring, or restriction behind internal-only headers?

2. **Fuel Gauge Reading Quirks:**
   - In `telemetry_adapter.c`, we query current via `furi_hal_power_get_battery_current(FuriHalPowerICFuelGauge)`.
   - *Question:* On certain hardware revisions (or specific BQ27220 / MAX17055 silicon variants), are there known operating regimes where reported current drops to 0.0mA or suffers from sign inversion during low-current trickle charging? Is there a preferred smoothing or calibration convention used in official tools?

---

## 2. Long-Lived Background Sampling Architecture

3. **Background Sampling Strategy for External FAPs:**
   - Battery Guardian currently samples telemetry only while the application is active in the foreground (`app.c`). When the user launches another application (e.g. Sub-GHz or NFC), sampling ceases.
   - *Question:* Is there an accepted community or firmware pattern for external applications to maintain a lightweight background thread (e.g. waking every 30 seconds to append a sample to a journal) without interfering with time-critical radio protocols or violating Flipper application lifecycle expectations?
   - *Question:* Or does the maintenance team strongly prefer that all background telemetry services reside strictly in the firmware core (`applications/services/power/`)?

---

## 3. Storage Paths & File Conventions

4. **Persistent Journal Storage Location:**
   - The application writes its binary journal to `/ext/apps_data/battery_guardian/journal.bin`.
   - *Question:* Is `/ext/apps_data/<appid>/` the canonical directory structure for external FAP persistent storage across all official firmware and catalog distributions?
   - *Question:* Does the storage subsystem have recommendations regarding write buffering sizes (e.g. 512-byte sector alignment) to maximize SD card flash endurance and minimize write latency during background ticks?

---

## 4. Hardware Abstraction & Capability Model

5. **Passive Fail-Closed Default:**
   - In `v1.0.0-rc1`, the Charger HAL (`core/charger_hal.c`) unconditionally refuses active charge control commands and advertises zero control capabilities in the production binary.
   - *Question:* Does the maintenance team endorse this passive fail-closed boundary for initial App Catalog release, leaving active charge management as an opt-in experimental build flag until broad community hardware testing is logged?

6. **PMIC Thermal Interaction:**
   - Battery Guardian implements software thermal cutoff at 45.0°C.
   - *Question:* The Texas Instruments BQ25896 PMIC also features internal autonomous hardware TS (thermistor) monitoring and JEITA profile regulation. Does software-commanded charge suppression interact cleanly with internal PMIC autonomous thermal throttling, or are there priority conflicts observed in hardware bench trials?

---

## 5. Potential Future Firmware Contribution

7. **Ecosystem Placement:**
   - In accordance with [CONTRIBUTING.md](https://github.com/flipperdevices/flipperzero-firmware/blob/dev/CONTRIBUTING.md), we have kept Battery Guardian as an external FAP.
   - *Question:* If community testing proves the Coulombic capacity estimation and confidence scoring models to be highly accurate across degraded battery packs, would maintainers be interested in upstreaming the core mathematical estimator (`phase2/estimator.c`, `phase2/confidence.c`) into `services/power` as an optional background health calculator?
