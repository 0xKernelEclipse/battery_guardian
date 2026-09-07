# Hardware Bench Validation Manual (Physical Verification Protocol)

> [!CAUTION]
> **PHYSICAL HARDWARE VALIDATION: NOT PERFORMED**
> 
> The development team and automated CI pipelines have **NOT executed active charge control commands on physical Flipper Zero hardware**. All active charge-limiting algorithms (`BALANCED` 80% target, `LIFESPAN` 60% target, charge suppression) have been validated exclusively in **host software simulation and property-based fuzz testing (100,000 transitions)**.
>
> In the production ARM binary (`dist/battery_guardian-v1.0.0-rc1.fap`), the Charger HAL operates in a strictly **PASSIVE and FAIL-CLOSED** mode (monitoring only, zero register writes).
>
> Physical hardware bench validation as specified below must be completed before any active charge control is unlocked for end users.

---

## 1. Objective & Scope

This manual defines the electrical engineering bench protocol required to validate Battery Guardian's telemetry acquisition, safety state machine, and simulated charging policies against real physical silicon:
- **STMicroelectronics STM32WB55RG** (Core application MCU)
- **Texas Instruments BQ25896** (Switching battery charger PMIC)
- **Texas Instruments BQ27220 / Maxim MAX17055** (Fuel gauge IC)

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

## 4. Execution Order & Safety Progression

Bench testing begins strictly with **passive observation** before progressing toward electrical fault injection and controlled actuation:

1. **Protocol B-01:** Baseline Telemetry Accuracy Validation (Passive Observation)
2. **Protocol B-04:** Fast USB VBUS Contact Bounce & Disconnect (Passive Power State)
3. **Protocol B-05:** Fuel Gauge I2C Bus Fault Injection (Passive Fault Handling)
4. **Protocol B-06:** Simulated Degraded/Swollen Battery Tracking (Multi-Session Observation)
5. **Protocol B-03:** Thermal Excursion Emergency Cutoff (Safety Gate)
6. **Protocol B-02:** Charge Suppression Electrical Response (Active Control Trial — Final Gate)

---

## 5. Detailed Bench Validation Protocols

### Protocol B-01: Baseline Telemetry Accuracy Validation (Passive)
- **Purpose:** Confirm that normalized telemetry reported by `telemetry_adapter.c` matches calibrated 6.5-digit bench multimeters across standard operating ranges without controlling power.
- **Equipment:** Programmable DC supply, 6.5-digit DMM, programmable electronic load.
- **Setup:** Battery simulator connected to Flipper battery header via 4-wire Kelvin connections.
- **Procedure:**
  1. Set supply voltage sequentially to 3.500V, 3.700V, 3.850V, 4.000V, 4.150V, 4.200V.
  2. At each voltage step, compare Battery Guardian displayed voltage on Dashboard vs DMM measured voltage.
  3. Apply 200mA discharge load using programmable electronic load; record reported current vs DMM current shunt.
- **Expected Result:**
  - Voltage error $\le \pm 15\,\text{mV}$ across $3.20\text{V} \dots 4.20\text{V}$.
  - Current error $\le \pm 10\,\text{mA}$ across $-500\text{mA} \dots +1000\text{mA}$.
  - State of Charge matches Fuel Gauge lookup within $\pm 2\%$.
- **Failure Condition:** Voltage error $> 25\,\text{mV}$ or reported current sign inverted.
- **Evidence:** DMM calibration certificate, tabular voltage/current readings, photo of Flipper UI.
- **Recovery Procedure:** Return DC supply to 3.800V nominal; power-cycle Flipper Zero.

---

### Protocol B-04: Fast USB VBUS Contact Bounce & Disconnect (Passive)
- **Purpose:** Verify that physical USB contact chatter or abrupt cable disconnection never corrupts the SD card journal or latches an invalid charging state.
- **Equipment:** Power-Z inline monitor, mechanical USB-C break-out toggle switch, oscilloscope.
- **Setup:** Connect 5V USB-C supply through mechanical toggle switch to Flipper USB port.
- **Procedure:**
  1. Start Battery Guardian and confirm active journal logging.
  2. Mechanically cycle the USB toggle switch 10 times in 2 seconds to simulate contact bounce.
  3. Abruptly toggle to disconnect and leave disconnected.
  4. Inspect the SD card binary journal (`battery_guardian.log`).
- **Expected Result:**
  - Zero filesystem or journal frame corruption; CRC32 verifies all stored records.
  - State immediately transitions to `UNMANAGED` on disconnect.
  - Zero latching of charge suppression state.
- **Failure Condition:** Journal file truncation, missing records before disconnect, or persistent suppression flag after unplug.
- **Evidence:** Oscilloscope trace of VBUS bounce, journal recovery hex dump, RTT console log.
- **Recovery Procedure:** Re-mount SD card; execute `journal_recover()` self-test in app.

---

### Protocol B-05: Fuel Gauge I2C Bus Fault Injection (Passive Fault)
- **Purpose:** Verify fail-closed behavior when fuel gauge communication is disrupted by electrical noise or transient line faults.
- **Equipment:** Logic analyzer, open-drain NMOS transistor attached to TP16 (SDA) and GND.
- **Setup:** Probe TP15 (SCL) and TP16 (SDA) with Saleae logic analyzer; trigger open-drain transistor via pulse generator.
- **Procedure:**
  1. With app running normally, pulse TP16 low for 500ms to simulate I2C bus collision/jam.
  2. Observe Battery Guardian Diagnostics and Health views.
  3. Monitor Furi OS kernel stability.
- **Expected Result:**
  - `telemetry_adapter` detects failure and flags `TelemetryStateFault` (`sample->gauge_ok = false`).
  - Policy state machine transitions to `SAFETY_LOCKOUT`.
  - UI displays `REALITY [FAULT]`.
  - Main Flipper OS does not crash, freeze, or panic.
- **Failure Condition:** Application crash, FreeRTOS deadlock on I2C bus mutex, or failure to flag gauge fault.
- **Evidence:** Saleae I2C capture showing SDA pull-down, Furi RTT console log, UI fault screen photo.
- **Recovery Procedure:** Release SDA line; verify state machine recovers when communication resumes.

---

### Protocol B-06: Simulated Degraded/Swollen Battery Tracking (Multi-Session)
- **Purpose:** Verify that Battery Guardian correctly identifies severely degraded packs where nominal 2100 mAh profile is inaccurate, without user intervention.
- **Equipment:** Battery simulator with programmable source impedance ($R_s$).
- **Setup:** Configure battery simulator to emulate a degraded 750 mAh cell with $R_s = 450\,\text{m}\Omega$.
- **Procedure:**
  1. Execute 3 complete discharge sessions ($\Delta\text{SOC} \ge 25\%$) under 250mA simulated load.
  2. Allow Battery Guardian session state machine to record and qualify each session.
  3. Inspect Health View and History View.
- **Expected Result:**
  - Observed capacity reports true degraded capacity ($750 \pm 75\,\text{mAh}$), distinct from stock 2100 mAh factory table.
  - Multi-factor confidence score transitions to `Medium` or `High`.
  - Health view shows degraded percentage (~36%) and OLS trend.
- **Failure Condition:** Capacity remains locked to 2100 mAh or calculation overflows.
- **Evidence:** Battery simulator energy log vs Battery Guardian displayed capacity, session table photo.
- **Recovery Procedure:** Reconfigure battery simulator to nominal 2100 mAh profile.

---

### Protocol B-03: Thermal Excursion Emergency Cutoff (Safety Gate)
- **Purpose:** Verify that cell temperature excursion above safe operating limits ($>45^\circ\text{C}$) triggers immediate charging lockout and suppression request.
- **Equipment:** Peltier temperature stage, Type-K thermocouple, thermal chamber, Power-Z monitor.
- **Setup:** Flipper placed on temperature-controlled plate with thermocouple affixed to battery pouch center. Unit connected to 5V USB charging.
- **Procedure:**
  1. With unit charging at 25°C, ramp Peltier stage temperature at $1^\circ\text{C}/\text{min}$ toward 48°C.
  2. Monitor Battery Guardian Safety FSM and VBUS current.
  3. Note temperature at which transition to `SAFETY_LOCKOUT` occurs.
  4. Allow plate to cool to 35°C and verify hysteresis recovery path.
- **Expected Result:**
  - FSM enters `SAFETY_LOCKOUT` within $<1000\,\text{ms}$ of thermistor exceeding 45.0°C.
  - Charge suppression requested.
  - Recovery does not occur until temperature falls below 40.0°C (hysteresis band).
- **Failure Condition:** Unit continues charging at $>45.5^\circ\text{C}$ or state oscillates rapidly near 45°C.
- **Evidence:** Temperature vs current logging chart, thermal camera image, RTT log.
- **Recovery Procedure:** Cut USB power immediately; remove unit to ambient cooling plate.

---

### Protocol B-02: Charge Suppression Electrical Response (Active Control Trial)
- **Purpose:** Measure the exact electrical transient, register state, and thermal response when active charge suppression is commanded on physical silicon.
- **Equipment:** Oscilloscope (current probe + voltage probe on VBAT), Saleae logic analyzer, Power-Z inline monitor.
- **Setup:** Current probe clamped around VBUS wire; oscilloscope triggered on falling edge of current; I2C sniffing TP15/TP16.
- **Precondition:** **Only executed after Protocols B-01, B-04, B-05, B-06, and B-03 have passed completely.** Custom test build with `BG_ENABLE_ACTIVE_CHARGE_CONTROL` compiled.
- **Procedure:**
  1. Connect USB-C power (5.0V). Verify nominal charging at ~400–500mA.
  2. Command charge suppression via test firmware interface (`furi_hal_power_suppress_charge_enter()`).
  3. Capture oscilloscope trace of VBUS current and VBAT voltage during cutoff.
  4. Sniff I2C1 bus to confirm BQ25896 register 0x03 (`CHG_CONFIG` bit 4) written correctly.
  5. Command charge resume (`furi_hal_power_suppress_charge_exit()`) and observe re-engagement.
- **Expected Result:**
  - VBUS current drops from charging current (>400mA) to quiescent (<15mA) in $< 50\,\text{ms}$.
  - Zero inductor ringing, zero voltage spike on VBAT exceeding 4.25V.
  - No FreeRTOS task hangs or system lockups.
- **Failure Condition:** Current fails to drop, voltage spike $>4.30\text{V}$, or PMIC stops responding on I2C.
- **Evidence:** Oscilloscope screenshot showing current step-down, Saleae I2C transaction decode, Power-Z trace.
- **Recovery Procedure:** Disconnect USB-C cable immediately; cycle battery simulator power.

---

## 6. Safety Hazards & Mitigation Summary

> [!WARNING]
> 1. Conduct all bench tests inside an explosion-resistant LiPo safety bag.
> 2. Maintain Class D / CO2 fire suppression apparatus nearby.
> 3. Do not test damaged, swollen, or punctured battery packs.
> 4. Never exceed 4.25V on the battery terminal or 5.5V on VBUS.
