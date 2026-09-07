# Battery Guardian — Hardware Bench Evidence Report Template

Use this template to record observations, oscilloscope captures, and multimeter logs during execution of physical hardware bench protocols (**B-01 through B-06**). Every bench execution must produce a completed instance of this form.

---

## Hardware Test Report Header

| Field | Value |
|---|---|
| **Test ID** | `B-01` / `B-02` / `B-03` / `B-04` / `B-05` / `B-06` |
| **Date & Time (UTC)** | `YYYY-MM-DD HH:MM:SS` |
| **Device Model & Hardware Rev** | e.g. `Flipper Zero F7B5C3 (Standard)` |
| **Firmware Version & Git Hash** | e.g. `Release 0.101.2 / commit a1b2c3d` |
| **Battery Guardian Build** | `v1.0.0-rc1 (SHA-256: 177A5FE1...)` |
| **Battery Pack Condition** | e.g. `Stock 2100 mAh Li-ion (New / Nominal)` or `Degraded Pack` |
| **Ambient Temperature & Humidity** | e.g. `23.5°C / 45% RH` |
| **Reviewer / Test Engineer** | Name & GitHub handle |

---

## 1. Test Setup & Instrumentation

### 1.1 Instruments Used
- **Multimeter / DMM:** Model, calibration date, serial number.
- **Power Supply / Battery Simulator:** Model, voltage limit, current compliance setting.
- **Oscilloscope:** Model, probe attenuation (1X/10X), bandwidth limit setting.
- **Inline Power Monitor:** Model, firmware revision.
- **Logic Analyzer:** Model, sampling rate (e.g. 25 MSa/s).

### 1.2 Physical Wiring & Probe Points
- *Describe Kelvin connections, ground clip placement, current clamp direction, and thermocouple attachment point.*

---

## 2. Experimental Procedure Log

| Step # | Time (s) | Commanded Action / Stimulus | Expected System Response |
|---|---|---|---|
| 1 | 0.00 | e.g. Apply 5.00V to VBUS via USB-C | VBUS detected, charging current ramps to ~450mA |
| 2 | 30.00 | e.g. Trigger charge suppression | VBUS current steps down to <15mA |
| 3 | ... | ... | ... |

---

## 3. Data Collection & Measurements

| Parameter | Nominal / Expected | Measured Value | Absolute Deviation | Relative Error (%) | Result |
|---|---|---|---|---|:---:|
| **Pack Voltage ($V_{\text{BAT}}$)** | `3.800 V` | `3.804 V` | `+0.004 V` | `+0.10%` | PASS |
| **Charging Current ($I_{\text{CHG}}$)**| `+450 mA` | `+442 mA` | `-8 mA` | `-1.77%` | PASS |
| **Cutoff Transient Time** | `< 50 ms` | `18.4 ms` | `-` | `-` | PASS |
| **Thermistor Reading** | `24.0°C` | `24.2°C` | `+0.2°C` | `+0.83%` | PASS |
| **Journal File Integrity** | 0 CRC errors | 0 CRC errors | 0 | 0% | PASS |

---

## 4. Oscilloscope & Analyzer Captures

*Insert or link waveform captures and logic analyzer traces:*
- **Capture 1:** VBUS current step-down on suppression trigger (`.png` / `.csv`).
- **Capture 2:** I2C1 bus transaction log decoding PMIC register `0x03` read/write.
- **Capture 3:** Thermal excursion cutoff timing trace.

---

## 5. Raw Observations & Behavioral Notes

- *Did the device reboot, hang, or exhibit display flicker during power transients?*
- *Did the Flipper UI reflect the exact expected state (e.g. `REALITY (WAIT)`, `Ctrl: Passive`)?*
- *Were any unexpected warnings or error messages logged over USB CDC / SWD?*

---

## 6. Pass / Fail Evaluation

- **Criteria Met:** [ ] YES  [ ] NO
- **Final Result:** **PASS** / **FAIL**
- **Discrepancies / Anomalies Observed:** *(Document any unexpected variance or safety boundary breaches)*

---

## 7. Sign-off

- **Test Engineer Signature:**  
- **Reviewer Approval:**  
- **Date:**  
