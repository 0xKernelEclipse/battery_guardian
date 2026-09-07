# Hardware Bench Validation Manual (Physical Verification Protocol)

> [!CAUTION]
> **PHYSICAL HARDWARE VALIDATION: NOT PERFORMED**
> The authors and automated validation pipelines have **NOT executed active charge control commands on physical Flipper Zero hardware**. All active charge-limiting algorithms (`BALANCED` 80% target, `LIFESPAN` 60% target, charge suppression) have been validated exclusively in **host software simulation and property-based fuzz testing (100,000 transitions)**.
>
> In the production ARM binary (`dist/battery_guardian.fap`), the Charger HAL operates in a strictly **PASSIVE and FAIL-CLOSED** mode (monitoring only, zero register writes).
>
> Physical hardware bench validation as specified below must be completed before any active charge control is ever unlocked for end users.

---

## 1. Objective & Scope

This manual defines the rigorous, reproducible electrical engineering bench protocol required to validate Battery Guardian's telemetry acquisition, safety state machine, and simulated charging policies against real physical silicon (STMicroelectronics STM32WB55RG, Texas Instruments BQ25896 charger PMIC, and Texas Instruments BQ27220 / Maxim MAX17055 fuel gauge).

---

## 2. Required Bench Equipment

| Instrument | Specification | Purpose |
|---|---|---|
| **Target Device** | Flipper Zero (F7B5C3 or newer, official battery) | Silicon under test |
| **Battery Simulator / DC Power Supply** | Keithley 2281S, Keysight N6705C, or Korad KD3005D (4-wire sense) | Replaces chemical cell with controlled voltage/source impedance |
| **Digital Storage Oscilloscope** | 100MHz+, 4-channel, 1 GSa/s (e.g. Rigol DS1054Z / Siglent SDS1104X-E) | Probing VBUS, VBAT, and PMIC switching waveforms |
| **Logic Analyzer** | 24MHz+, 8-channel (e.g. Saleae Logic Pro 8) | Sniffing internal I2C1 bus (TP15=SCL, TP16=SDA) |
| **Inline USB-C Power Monitor** | Power-Z KM003C or AVHzY CT-3 | High-rate logging of VBUS voltage, current, and ripple |
| **Temperature Sensor** | Calibrated Type-K Thermocouple + thermal tape | Affixed directly to battery pouch center |
| **Thermal Chamber / Peltier Stage** | 0°C to 65°C controlled temperature plate | Thermal fault and temperature coefficient verification |
| **SWD Debug Probe** | ST-LINK V3 / J-Link Base | SWD flashing, FreeRTOS thread inspection, RTT logging |

---

## 3. Test Bench Setup Diagram

```
+-------------------------------------------------------------------------+
|                              BENCH SETUP                                |
|                                                                         |
|  +--------------------+                   +--------------------------+  |
|  | Programmable DC    |-- 4-wire (VBAT) ->| Flipper Battery Header   |  |
|  | Battery Simulator  |                   | (Thermistor line intact) |  |
|  +--------------------+                   +--------------------------+  |
|                                                         |               |
|  +--------------------+                   +--------------------------+  |
|  | USB-C 5V Supply    |-- USB-C Cable --->| Power-Z Inline Monitor   |  |
|  +--------------------+                   +--------------------------+  |
|                                                         | (VBUS)        |
|                                                         v               |
|                                           +--------------------------+  |
|  +--------------------+                   | Flipper Zero USB-C Port  |  |
|  | Saleae Logic (I2C) |<-- TP15/TP16 -----+--------------------------+  |
|  +--------------------+                                 |               |
|                                           +--------------------------+  |
|  +--------------------+                   | Thermocouple on Cell     |  |
|  | Thermocouple Meter |<-- Thermal Tape --+--------------------------+  |
|  +--------------------+                                                 |
+-------------------------------------------------------------------------+
```

---

## 4. Step-by-Step Bench Validation Protocols

### Protocol B-01: Baseline Telemetry Accuracy Validation
* **Objective**: Confirm that `core/telemetry_adapter.c` readings match calibrated 6.5-digit bench multimeters across the full operating range.
* **Procedure**:
  1. Connect battery simulator to Flipper battery terminal. Set voltage to 3.500V, 3.700V, 3.850V, 4.000V, 4.150V, 4.200V.
  2. At each voltage step, record Battery Guardian displayed voltage on Dashboard vs DMM measured voltage.
  3. Apply 200mA discharge load using external programmable electronic load. Record reported current vs DMM shunt current.
* **Pass Criteria**:
  - Voltage error $\le \pm 15\,\text{mV}$ across $3.20\text{V} \dots 4.20\text{V}$.
  - Current error $\le \pm 10\,\text{mA}$ across $-500\text{mA} \dots +1000\text{mA}$.
  - State of Charge matches Fuel Gauge lookup within $\pm 2\%$.

### Protocol B-02: Charge Suppression Electrical Response (Active Control Trial)
* **Objective**: Measure the exact electrical transient and register state when charge suppression is commanded.
* **Procedure**:
  1. Connect USB-C power (5.0V). Verify nominal charging at ~400–500mA.
  2. Command charge suppression via test firmware build.
  3. Observe VBUS current waveform on oscilloscope (trigger on falling edge of current).
  4. Sniff I2C1 bus to capture BQ25896 register 0x03 (`CHG_CONFIG` bit 4).
* **Pass Criteria**:
  - VBUS current drops from charging current (>400mA) to quiescent (<15mA) in $< 50\,\text{ms}$.
  - Zero inductor ringing, zero voltage overshoot on VBAT $> 4.25\,\text{V}$.
  - No unexpected resets or FreeRTOS task hangs.

### Protocol B-03: Thermal Excursion Emergency Cutoff
* **Objective**: Verify that cell temperature excursion forces immediate charging lockout.
* **Procedure**:
  1. With unit charging at room temperature (25°C), slowly heat Peltier stage to 46°C.
  2. Monitor Battery Guardian Safety State Machine.
  3. Verify transition to `ChargeStateFault` and trigger of charge suppression.
  4. Allow stage to cool to 35°C.
  5. Verify two-stage recovery: `ChargeStateFault` $\to$ `ChargeStateRecovery` $\to$ `ChargeStateChargingAllowed`.
* **Pass Criteria**:
  - Charge cutoff occurs within $< 1000\,\text{ms}$ of thermistor crossing 45.0°C threshold.
  - VBUS current drops to standby level immediately upon cutoff.
  - State does not oscillate; recovers cleanly only after hysteresis clear.

### Protocol B-04: Fast USB VBUS Contact Bounce & Disconnect
* **Objective**: Verify that physical USB contact chatter or sudden unplug never results in a latched charge state or storage corruption.
* **Procedure**:
  1. While unit is in `ChargeStateChargingAllowed` and actively writing journal records, mechanically cycle the USB-C plug rapidly (10 cycles in 2 seconds).
  2. Check file integrity on microSD card (`storage/journal.c`).
  3. Verify Safety State Machine immediately forces `ChargeStateUnmanaged` on disconnect.
* **Pass Criteria**:
  - Zero filesystem corruption, zero unreadable records in `battery_guardian.log`.
  - State is strictly `ChargeStateUnmanaged` when VBUS is removed.

### Protocol B-05: Fuel Gauge I2C Bus Fault Injection
* **Objective**: Verify fail-closed behavior when fuel gauge communication is disrupted.
* **Procedure**:
  1. While running, momentarily short I2C1 SDA (TP16) to GND for 500ms using an open-drain transistor.
  2. Observe system response via SWD / RTT logs.
* **Pass Criteria**:
  - `telemetry_adapter` flags `TelemetryStateFault` and `sample->gauge_ok = false`.
  - Battery Guardian immediately transitions to `ChargeStateFault` (fail-closed).
  - Main Flipper OS does not panic or hang; recovers once I2C bus is released.

### Protocol B-06: Simulated Degraded/Swollen Battery (GitHub Issue #4083 Replication)
* **Objective**: Verify that Battery Guardian correctly identifies severely degraded packs where nominal 2100 mAh profile is inaccurate.
* **Procedure**:
  1. Configure battery simulator with high source resistance ($R_s = 450\,\text{m}\Omega$) and rapid voltage drop under 300mA load (equivalent to a 750 mAh degraded cell).
  2. Run unit through 3 complete discharge cycles.
  3. Inspect Battery Guardian Health view.
* **Pass Criteria**:
  - Observed capacity reports true capacity (~750 mAh $\pm 10\%$), distinct from stock gauge profile.
  - Trend correctly transitions to `DECLINING` or degraded alert.
  - No mathematical overflow or divide-by-zero errors.

---

## 5. Physical Safety Hazards & Mitigation

> [!WARNING]
> **Lithium-Ion Safety Precautions**
> 1. Always perform battery electrical tests inside an explosion-proof LiPo charging bag or steel enclosure.
> 2. Have Class D or CO2 fire extinguishing equipment accessible.
> 3. Discard any pouch battery exhibiting deformation, swelling, or odor immediately.
> 4. Do NOT attempt to charge cells whose terminal voltage has collapsed below 2.5V.
