# Battery Guardian

Battery Guardian is a Flipper Zero app for recording battery telemetry and building an observed capacity estimate from charge and discharge sessions.

The app reads the values already exposed by the Flipper power HAL. It keeps its own session history so it can compare observed discharge behavior over time with the health value reported by the fuel gauge. It does not replace the gauge model, and it does not claim that its estimate is a laboratory measurement.

The production charger interface is passive. The app can evaluate a charge policy and show what that policy would request, but it does not write PMIC charge-control registers or stop charging on physical hardware.

## What it reads

The telemetry adapter reads:

- Battery voltage, in volts.
- Battery current, in amps. Positive values mean charging and negative values mean discharging.
- Battery temperature, in Celsius.
- State of charge, in percent.
- Remaining, full, and design capacity when the HAL provides them.
- Fuel-gauge status and the gauge-reported health percentage.
- Charging state, charge completion, USB voltage, and USB presence.

Each sample also has validity flags, an operational state, and a monotonic timestamp. The adapter extends the Flipper tick counter across its 32-bit rollover so session calculations do not depend on wall-clock time.

## How the estimate works

The worker samples telemetry while the app is open. The session manager groups samples into charging, discharging, and idle sessions. A discharge session needs enough samples, a meaningful SOC change, valid sensor data, and no temperature excursion before it can contribute to an estimate.

The estimator applies a minimum 15 percentage-point SOC drop. It integrates the discharge current over the session and divides that amount by the observed SOC change to produce a capacity candidate. It keeps up to 16 accepted candidates, rejects candidates that are too far from the existing median, and uses the median as the observed capacity estimate.

Confidence starts as insufficient until at least three accepted candidates exist. It then considers the number of accepted candidates, the spread between them, and rejected candidates. The degradation view uses the accepted candidates to calculate a capacity trend once enough history exists.

The app shows two different health values:

- **Gauge health:** the percentage returned by `furi_hal_power_get_bat_health_pct()`.
- **Observed health:** the capacity estimate compared with the configured reference capacity.

Both are estimates made from different data. The gauge value comes from the fuel gauge; the observed value comes from Battery Guardian's recorded sessions.

## Charge policy

The policy engine supports these targets:

- Unmanaged, which leaves charging alone.
- Balanced, with an 80% target.
- Lifespan, with a 60% target.
- Full, with a 100% target.
- Custom, with a user-selected target and hysteresis.

The policy has separate states for unmanaged charging, charging allowed, target reached, charge suppressed, recovery, and fault. It checks USB presence, SOC, voltage, temperature, and gauge status. A USB disconnect returns the policy to unmanaged charging. Invalid sensor data moves it to a fault state.

In the host mock, charge suppression requests can be accepted so the policy transitions can be tested. In the production FAP, the charger HAL does not advertise charge-control support and rejects those requests. A policy state such as `CHARGE_SUPPRESSED` therefore describes the requested policy state, not a measured change in physical charging current.

## Storage

The journal is stored at:

```text
/ext/apps_data/battery_guardian.log
```

It contains a header followed by typed records for telemetry, sessions, events, and saved estimates. Each record has a CRC for its payload. Startup recovery keeps the valid prefix of a partially written file, and the journal stops accepting records at 512 KB. The sample buffer is bounded, so it evicts its oldest sample when it fills.

Saved estimates are restored when the app starts. The restore path brings back the capacity, reference capacity, confidence, session counts, trend, and timestamp used by the health view.

## User interface

The app includes views for:

- **Dashboard:** current voltage, current, temperature, SOC, charging state, and estimate status.
- **Health:** gauge health, observed capacity, observed health, confidence, and trend.
- **History:** recent telemetry and capacity history.
- **Sessions:** completed and active charge or discharge sessions.
- **Diagnostics:** sensor validity, counters, journal state, and current policy state.

Sampling pauses when the app exits back to the Flipper desktop.

## Build and run

Install the Flipper build tool and SDK:

```bash
python -m pip install --upgrade ufbt
ufbt update
```

Build the FAP:

```bash
ufbt clean
ufbt
```

The output is `dist/battery_guardian.fap`. With a Flipper connected, `ufbt launch` builds, copies, and launches the app.

## Tests

Run the native test suite without a Flipper:

```bash
python tests/run_tests.py
```

The runner uses Zig, Clang, or GCC. The tests cover telemetry validation, sampling, session transitions, current integration, journal writes and recovery, estimation, confidence and degradation calculations, charge-policy transitions, malformed input, and persistence edge cases. The current run reports 124 passing unit and integration checks plus 100,000 charge-policy property transitions.

These tests use mocks and synthetic data. They do not measure a real battery, verify sensor calibration, or prove that a PMIC command changes charging safely.

## Hardware status

Physical hardware validation has not been completed. The current FAP only observes the power HAL and evaluates policy state. It does not control charging.

Before active charge control is considered, the project needs instrumented tests for telemetry accuracy, USB disconnects, fuel-gauge faults, controlled discharge sessions, temperature limits, and the electrical response to charge suppression. [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md) lists those checks.

## Repository layout

- `core/`: telemetry, sessions, events, diagnostics, and the charger interface.
- `phase2/`: capacity estimation, confidence, degradation, health, and charge policy.
- `storage/`: the binary journal and recovery code.
- `model/`: worker-thread state and GUI snapshots.
- `gui/`: scenes and views.
- `tests/`: native tests, mocks, and simulation traces.

The shorter supporting documents are [ARCHITECTURE.md](ARCHITECTURE.md), [BUILD.md](BUILD.md), [TESTING.md](TESTING.md), [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md), and [SECURITY.md](SECURITY.md).

AI tools were used during development and documentation. The repository owner is responsible for reviewing the implementation, test results, and safety claims.

## License

MIT. See [LICENSE](LICENSE).
