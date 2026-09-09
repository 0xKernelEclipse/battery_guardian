# Battery Guardian

Battery Guardian is a Flipper Zero app for collecting battery telemetry and building an observed capacity estimate from charge and discharge sessions.

It reads the values exposed by the Flipper power HAL, keeps its own session history, and shows that history alongside the health percentage reported by the fuel gauge. The two values come from different sources. Battery Guardian does not replace the gauge model and does not claim laboratory accuracy.

> **Current status:** The software is working and the host test suite passes. The production charger interface is passive, and physical battery or charge-control testing has not been completed.

## At a glance

| | |
|---|---|
| Platform | Flipper Zero, Target 7, API 87.1 |
| App type | External FAP, Tools category |
| Storage | `/ext/battery_guardian.log` |
| Charger control | Disabled in the production FAP |
| Host tests | 128 unit and integration checks |
| Policy property test | 100,000 transitions |
| License | MIT |

## Quick start

### Build the FAP

Install the Flipper build tool and SDK:

```bash
python -m pip install --upgrade ufbt
ufbt update
```

Build the application:

```bash
ufbt clean
ufbt
```

The output is `dist/battery_guardian.fap`. With a Flipper connected, `ufbt launch` builds, copies, and launches the app.

### Run the host tests

The test runner uses Zig, Clang, or GCC and does not require a Flipper:

```bash
python tests/run_tests.py
```

For the combined test, target build, and package check:

```bash
python scripts/validate.py
```

## What the app reads

The telemetry adapter reads voltage, current, temperature, SOC, charging state, USB state, fuel-gauge status, and the gauge-reported health percentage through the Flipper power HAL. When available, it also records remaining, full, and design capacity values.

Every sample carries validity flags, an operational state, and a monotonic timestamp. The adapter extends the Flipper tick counter across its 32-bit rollover, so session timing does not depend on wall-clock time.

## Capacity and health estimates

The worker samples telemetry while the app is open. The session manager groups samples into charging, discharging, and idle sessions. A discharge session must have enough samples, a meaningful SOC change, valid sensor data, and no temperature excursion before it can contribute to capacity estimation.

The estimator accepts sessions with at least a 15 percentage-point SOC drop. It integrates discharge current over the session and divides that amount by the observed SOC change to produce a capacity candidate. It keeps up to 16 accepted candidates, rejects candidates that are far from the current median, and uses the median as the observed capacity estimate.

Confidence is insufficient until at least three candidates exist. After that, it considers the number of accepted candidates, the spread between candidates, and rejected candidates. The degradation view calculates a capacity trend after enough history has accumulated.

The app shows two health values:

- **Gauge health:** returned by `furi_hal_power_get_bat_health_pct()`.
- **Observed health:** calculated from the observed capacity estimate and reference capacity.

Both are estimates. The gauge value comes from the fuel gauge; the observed value comes from Battery Guardian's recorded sessions.

## Charge policy

The policy engine supports these targets:

- **Unmanaged:** leave charging alone.
- **Balanced:** 80% target.
- **Lifespan:** 60% target.
- **Full:** 100% target.
- **Custom:** user-selected target and hysteresis.

The policy tracks unmanaged charging, charging allowed, target reached, charge suppressed, recovery, and fault states. It checks USB presence, SOC, voltage, temperature, and gauge status. A USB disconnect returns the policy to unmanaged charging. Invalid sensor data moves it to a fault state.

The host mock accepts suppression requests so policy transitions can be tested. The production charger HAL does not advertise charge-control support and rejects those requests. A `CHARGE_SUPPRESSED` policy state therefore describes the requested state, not a measured change in physical charging current.

## Storage

The journal is stored at:

```text
/ext/battery_guardian.log
```

It contains a header and typed records for telemetry, sessions, events, and saved estimates. Each record has a CRC for its payload. Startup recovery keeps the valid prefix of a partially written file, and the journal stops accepting records at 512 KB. The in-memory sample buffer is bounded and evicts its oldest sample when full.

Saved estimates are restored when the app starts, including capacity, reference capacity, confidence, session counts, trend, and timestamp.

## User interface

- **Dashboard:** current voltage, current, temperature, SOC, charging state, and estimate status.
- **Health:** gauge health, observed capacity, observed health, confidence, and trend.
- **History:** recent telemetry and capacity history.
- **Sessions:** completed and active charge or discharge sessions.
- **Diagnostics:** sensor validity, counters, journal state, and policy state.

Sampling pauses when the app exits to the Flipper desktop.

## Hardware boundary

The current FAP only observes the power HAL and evaluates policy state. It does not write PMIC charge-control registers or stop charging.

Before active charge control is considered, the project needs instrumented tests for:

1. Telemetry accuracy against calibrated instruments.
2. USB disconnects and contact bounce while logging.
3. Fuel-gauge communication faults.
4. Controlled discharge sessions and capacity estimates.
5. Temperature limits and recovery.
6. Electrical response to charge suppression.

The planned checks are listed in [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md). This repository contains no physical test results yet.

## Repository layout

```text
core/       telemetry, sessions, events, diagnostics, charger interface
phase2/     estimator, confidence, degradation, health, charge policy
storage/    binary journal and recovery
model/      worker-thread state and GUI snapshots
gui/        scenes and views
tests/      native tests, mocks, and simulation traces
```

Useful project documents:

- [ARCHITECTURE.md](ARCHITECTURE.md), code paths and data flow
- [BUILD.md](BUILD.md), build prerequisites and commands
- [TESTING.md](TESTING.md), host test scope and limitations
- [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md), physical test plan
- [SECURITY.md](SECURITY.md), storage and input-handling concerns
- [Releases](https://github.com/0xKernelEclipse/battery_guardian/releases), packaged versions and FAP downloads

AI tools were used during development and documentation. The repository owner is responsible for reviewing the implementation, test results, and safety claims.

## License

MIT. See [LICENSE](LICENSE).
