# Battery Guardian

Battery Guardian is an embedded battery intelligence, telemetry observation, capacity estimation, and safety monitoring application for the Flipper Zero.

## Features

- **Continuous Telemetry Observation:** Captures and normalizes physical SI signals (voltage, current, temperature, state of charge).
- **Session-Aware Capacity Learning:** Discards single-point voltage heuristics in favor of Coulombic integration across qualified discharge sessions.
- **Statistical Filtering & Confidence Scoring:** Applies median filtering over historical sessions and computes multi-factor confidence ratings (High, Medium, Low, Insufficient).
- **Long-Term Degradation Tracking:** Derives capacity loss trajectories using linear regression.
- **Crash-Resilient Journaling:** Append-only binary journal with per-record CRC32 verification and 512 KB storage quota.
- **Formal Safety State Machine:** Evaluates charge policies with hysteresis guards and thermal lockouts.
- **Passive Fail-Closed Hardware Boundary:** Strict Hardware Abstraction Layer running in passive fail-closed mode (zero PMIC register writes in production).

## User Interface Views

- **Dashboard:** Real-time SI telemetry, battery state, and active power mode.
- **Health & Intelligence:** True observed capacity, nominal comparison, and wear trend.
- **History:** Visual historical discharge graph with median filtering.
- **Sessions:** Chronological log of qualified discharge sessions.
- **Diagnostics:** Sensor validity flags, crash counters, and safety FSM status.

## Safety Notice

Physical hardware bench validation has not yet been performed. The production charger HAL operates in a strictly passive, fail-closed configuration without issuing register writes to hardware charging registers.
