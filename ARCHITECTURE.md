# Battery Guardian Architecture

## 1. System Overview

Battery Guardian is engineered with strict modular decomposition, hard concurrency boundaries, and deterministic data flow. The system architecture enforces a clean separation between high-level intelligence and physical hardware access through dedicated abstraction adapters:

```
+-------------------------------------------------------------------------+
|                       BATTERY GUARDIAN ARCHITECTURE                     |
+-------------------------------------------------------------------------+
|  [PRESENTATION LAYER]                                                   |
|  GUI Views (Dashboard, Health, Diagnostics, Sessions, History, Events) |
|                                ^                                        |
|                          (Thread-Safe Snapshot Copy)                    |
|                                v                                        |
|  [MODEL & COORDINATION LAYER]                                           |
|  BatteryModel (Worker Thread) | HistoryModel | SessionModel | EventModel|
|                                ^                                        |
|                                v                                        |
|  [INTELLIGENCE & POLICY LAYER]                                          |
|  Estimator | Confidence | Outlier Rejection | Degradation | PolicyEng  |
|                                ^                                        |
|                                v                                        |
|  [STORAGE LAYER]                                                        |
|  Journal Engine (Crash-Resilient Append-Only Log, 512KB Quota)          |
|                                ^                                        |
|                                v                                        |
|  [HARDWARE ABSTRACTION LAYER]                                           |
|  Telemetry Adapter (SI Units)   |   Charger HAL (Passive Fail-Closed)   |
+---------------------------------+---------------------------------------+
|  [PLATFORM / HARDWARE LAYER]                                            |
|  Flipper Zero SDK (furi_hal_power.h, power.h, BQ25896, BQ27220)         |
+-------------------------------------------------------------------------+
```

---

## 2. Platform API Audit & Categorization

The official Flipper Zero SDK power interfaces (`targets/furi_hal_include/furi_hal_power.h` and `applications/services/power/power_service/power.h`) have been audited and categorized as follows:

| API Function | Classification | Architectural Constraint & Safety Behavior |
|---|---|---|
| `furi_hal_power_gauge_is_ok()` | `SUPPORTED` | Checks hardware fuel gauge alive status, profile integrity, and self-test. |
| `furi_hal_power_get_pct()` | `SUPPORTED` | Primary battery State-of-Charge (0-100%). |
| `furi_hal_power_is_charging()` | `SUPPORTED` | Read directly from BQ25896 charging status register. |
| `furi_hal_power_is_charging_done()` | `SUPPORTED` | Indicates full charge termination. |
| `furi_hal_power_get_battery_voltage(ic)` | `SUPPORTED` | Returns battery cell voltage in Volts (`float`). Fuel gauge IC is preferred. |
| `furi_hal_power_get_battery_temperature(ic)` | `SUPPORTED` | Returns pack temperature in Celsius (`float`). Fuel gauge NTC is preferred. |
| `furi_hal_power_get_usb_voltage()` | `SUPPORTED` | VBUS input voltage in Volts (`float`) used to verify physical USB connection. |
| `furi_hal_power_get_battery_remaining_capacity()` | `SUPPORTED` | Fuel gauge remaining capacity in mAh (`uint32_t`). |
| `furi_hal_power_get_battery_full_capacity()` | `SUPPORTED` | Fuel gauge full charge capacity in mAh (`uint32_t`). |
| `furi_hal_power_get_battery_design_capacity()` | `SUPPORTED` | Nominal factory design capacity in mAh (2100 mAh). |
| `furi_hal_power_get_battery_current(ic)` | `SUPPORTED WITH LIMITATIONS` | Fuel gauge provides bidirectional pack current (signed A); charger IC only provides charging input current. |
| `furi_hal_power_get_bat_health_pct()` | `SUPPORTED WITH LIMITATIONS` | Gauge reported SOH; static or inaccurate on clone/aged batteries. Battery Guardian observed health is preferred. |
| `furi_hal_power_suppress_charge_enter()` / `_exit()` | `SUPPORTED WITH LIMITATIONS` | Designed for Sub-1GHz low-noise operation; NOT validated for third-party charge cycling. **Bypassed in production FAP**. |
| `furi_hal_power_set_battery_charge_voltage_limit()` | `SUPPORTED WITH LIMITATIONS` | Manipulates volatile PMIC registers; unsafe without bench oscilloscope verification. **Bypassed in production FAP**. |
| Direct PMIC register read/write | `UNAVAILABLE` | Private internal HAL layer, not exposed in public SDK headers. |
| Raw ADC channels for battery sense | `UNAVAILABLE` | High-level HAL abstractions only. |
| PMIC Watchdog behavior under hold | `UNKNOWN` | Behavior of BQ25896 internal 160s watchdog under indefinitely held suppression. |

---

## 3. Telemetry Adapter Layer (`core/telemetry_adapter.h/.c`)

The Telemetry Adapter enforces strict physical SI unit normalization and isolates core intelligence from platform-specific quirks:

- **Strict SI Units**:
  - Voltage: Volts ($V$, `float`)
  - Current: Amperes ($A$, `float`, positive = charging, negative = discharging)
  - Temperature: Celsius ($^\circ C$, `float`)
  - State of Charge: Percent ($0 \dots 100\%$, `uint8_t`)
  - Timestamp: Monotonic milliseconds with 64-bit rollover compensation across 32-bit tick counter wraps ($2^{32}\,\text{ms} \approx 49.7\,\text{days}$).
- **Capability Bitmasks (`TELEMETRY_CAP_*`)**:
  Identifies whether the host platform supports voltage, current, temperature, SOC, capacities, USB voltage, or fuel gauge status.
- **Operational States (`TelemetryState`)**:
  - `TelemetryStateNormal`: All primary metrics ($V, I, T, \text{SOC}$) valid.
  - `TelemetryStatePartial`: Non-critical fields missing, safe to monitor.
  - `TelemetryStateFault`: Fuel gauge communication failure or hostile out-of-bounds readings.
  - `TelemetryStateUnavailable`: Subsystem offline.

---

## 4. Charger HAL Architecture & Passive Production Contract

The Charger HAL (`phase2/charger_hal.h`, `core/charger_hal.c`) implements an explicit capability-based contract:

```c
#define CHARGER_CAP_CHARGE_CONTROL  (1 << 0) // Hardware charge stop/start control
#define CHARGER_CAP_CHARGE_STATUS   (1 << 1) // Reading charger status
#define CHARGER_CAP_BATTERY_VOLTAGE (1 << 2) // Reading battery voltage
#define CHARGER_CAP_BATTERY_CURRENT (1 << 3) // Reading battery current
#define CHARGER_CAP_BATTERY_TEMP    (1 << 4) // Reading battery temperature
#define CHARGER_CAP_BATTERY_SOC     (1 << 5) // Reading battery SOC %
#define CHARGER_CAP_GAUGE_STATUS    (1 << 6) // Reading fuel gauge OK state
```

### Production Safety Contract (Fail-Closed Passive):
1. The production ARM FAP binary **never advertises `CHARGER_CAP_CHARGE_CONTROL`**.
2. If `charger_hal_request_charge_disable()` or `charger_hal_request_charge_enable()` is invoked on physical hardware, it logs:
   `[WARN][BatGuard] PASSIVE FAIL-CLOSED: Physical charge suppression rejected on hardware.`
   and returns `false` (safe refusal).
3. **Zero physical register writes** occur on real hardware. Active charge control algorithms are validated exclusively in software simulations.

---

## 5. Concurrency, Lock Hierarchy & Threading

```
+-------------------------------------------------------------------------+
|                              LOCK HIERARCHY                             |
+------------------------------------+------------------------------------+
|       GUI / Main Thread            |       Worker Thread                |
|   (ViewDispatcher / SceneManager)  |    (battery_model_worker_thread)   |
+------------------------------------+------------------------------------+
|                                    |                                    |
| 1. Dispatches user input           | 1. Reads Telemetry Adapter (Lock-Free)
| 2. Periodic view tick (500ms)      | 2. Evaluates Session Pipeline (Lock-Free)
| 3. Acquires Model Mutex (100ms)    | 3. Writes SD Journal (Lock-Free)   |
| 4. Copies snapshot in ~10 microseconds | 4. Evaluates Policy Engine (Lock-Free)
| 5. Releases Model Mutex            | 5. Evaluates Health Engine (Lock-Free)
| 6. Draws view on Canvas (Lock-Free)| 6. Acquires Model Mutex            |
|                                    | 7. Swaps BatteryModelSnapshot (~10us)|
|                                    | 8. Releases Model Mutex            |
|                                    | 9. Sleeps for adaptive interval    |
+------------------------------------+------------------------------------+
```

### Concurrency Invariants:
1. **Zero I/O Under Locks**: MicroSD disk I/O, CRC computations, and HAL polling are strictly forbidden inside `furi_mutex_acquire` / `release`.
2. **Deterministic Teardown Sequence**:
   - `battery_model_stop_polling()` sets `running = false` and joins the worker thread.
   - Event listeners are cleared (`event_set_listener(NULL, NULL)`).
   - GUI views are unregistered from `ViewDispatcher` and deallocated.
   - Journal buffers are flushed and `RECORD_STORAGE` is closed.
   - Zero callbacks can execute against deallocated memory.

---

## 6. Adaptive Sampling & Power Efficiency

To prevent battery drain from constant background wakeups, the sampling engine dynamically modulates its polling interval:

| Operational Mode | Polling Interval | Trigger Condition | Rationale |
|---|---|---|---|
| **Idle** | 30,000 ms (30 s) | Disconnected, pack current $>-0.05\,\text{A}$ | Maximizes MCU sleep time when idle. |
| **Active Discharging** | 10,000 ms (10 s) | Disconnected, pack current $\le -0.05\,\text{A}$ | Balances coulomb counter integration accuracy with power. |
| **Charging** | 5,000 ms (5 s) | USB connected and charging | Accurately tracks charge curve and termination. |
| **Anomaly** | 1,000 ms (1 s) | Temperature excursion or sensor fault | Rapid safety response during thermal/electrical events. |

---

## 7. Storage Engine & 512KB Quota Protection

The persistence layer (`storage/journal.c`) manages an append-only binary journal on microSD storage (`/ext/battery_guardian.log`):
- **Magic & Version**: Validated header (`0x42474A31`, format version 1). Future format versions are rejected safely without overwriting.
- **Strict File Quota**: Hard-capped at 512 KB (`JOURNAL_MAX_FILE_SIZE = 524288`). When reached, new records are dropped with a diagnostic warning to prevent storage exhaustion.
- **Streaming Recovery**: Crash recovery uses a 512-byte streaming buffer and 64-bit file offsets, supporting files $>64\,\text{KB}$ without 16-bit integer truncation.
