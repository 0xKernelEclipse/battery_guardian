# Simulation tests

These tests feed synthetic traces into the session, estimator, and charge-policy code. They cover normal discharges, degradation, outliers, bad data, temperature changes, and larger session counts.

They test software behavior. They do not measure a battery or charger. Those checks need physical hardware.

- `trace.c/h` builds the input traces.
- `simulator.c/h` replays them through the application code.
- `test_simulation.c` contains the scenarios.
