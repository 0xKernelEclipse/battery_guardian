# Testing

Run the native suite with:

```bash
python tests/run_tests.py
```

The runner compiles the C sources with Zig, Clang, or GCC and runs them without a Flipper. It covers telemetry validation, sampling, session transitions, current integration, journal writes and recovery, the capacity estimator, confidence and degradation calculations, charge-policy transitions, invalid inputs, and persistence edge cases.

The current run has 124 passing unit and integration checks and a 100,000-transition charge-policy property test. The test executable prints the totals at the end of each run.

The suite uses mocks and synthetic data. It does not measure a real battery, verify sensor calibration, or prove that a PMIC command changes charging safely. Those checks require physical hardware.
