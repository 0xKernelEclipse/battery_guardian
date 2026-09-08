# Battery Guardian

Battery Guardian is a Flipper Zero app that records battery readings while it is running. It keeps charge and discharge sessions, estimates capacity from discharge current and SOC change, and shows that estimate alongside the health value reported by the fuel gauge.

The app can evaluate charge policies, but the production charger interface is passive. It does not change the PMIC or stop charging on physical hardware.

## What it does

- Reads voltage, current, temperature, SOC, USB state, and gauge health through the Flipper power HAL.
- Tracks sessions while the app is open.
- Estimates capacity from discharge sessions with at least a 15 percentage-point SOC drop.
- Stores telemetry, sessions, events, and estimates in `/ext/apps_data/battery_guardian.log`.
- Shows current readings, gauge health, observed capacity, confidence, and degradation trend.
- Evaluates unmanaged, balanced, lifespan, full, and custom charge policies.

The gauge health value and the observed estimate are separate. The gauge value comes from `furi_hal_power_get_bat_health_pct()`. The observed value comes from Battery Guardian's session history. The app does not claim that either value is ground truth.

## How it works

```text
Flipper power HAL -> telemetry -> sessions -> estimator -> journal -> UI
                                      \-> charge policy -> passive charger HAL
```

The estimator integrates discharge current, divides by the observed SOC change, rejects large outliers, and keeps a bounded history. The journal uses fixed records with CRC checks and stops accepting samples at 512 KB.

## Current limitations

- Sampling stops when the app exits.
- The estimate needs qualifying sessions before it becomes useful.
- Physical charge-control testing has not been completed.
- Production charge control is disabled, so policy state does not mean that charging was physically stopped.
- The observed health value is an estimate from recorded sessions, not a replacement for the fuel gauge model.
- Host tests and simulated hardware do not prove electrical behavior on a Flipper.

## Build

Install `ufbt` and update its SDK:

```bash
python -m pip install --upgrade ufbt
ufbt update
```

Build the FAP:

```bash
ufbt clean
ufbt
```

Run the native test suite:

```bash
python tests/run_tests.py
```

The test runner uses Zig, Clang, or GCC. Set `CC` if needed.

## Files

- [ARCHITECTURE.md](ARCHITECTURE.md) describes the main code paths.
- [TESTING.md](TESTING.md) describes the host tests and their limits.
- [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md) lists the physical checks still needed.
- [BUILD.md](BUILD.md) has target-build details.
- [SECURITY.md](SECURITY.md) covers malformed data and bug reports.

## Status

The app and host tests are working. Hardware validation and independent review are still pending. AI tools were used during development and documentation; the repository owner is responsible for checking the code and the claims in this repository.

## License

MIT. See [LICENSE](LICENSE).
