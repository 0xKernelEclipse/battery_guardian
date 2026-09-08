# Battery Guardian: Simulation Harness

## Hardware Boundary Disclaimer

**WARNING: THIS SIMULATION FRAMEWORK VALIDATES SOFTWARE LOGIC ONLY.**

The test vectors and simulated traces in this directory **DO NOT PROVE**:
* Real battery capacity
* Real charger behavior
* Real power-management IC (PMIC) characteristics
* Real gauge accuracy

Those elements require physical hardware. This harness exercises the estimator and policy code against synthetic data.

Explicitly: **Synthetic traces validate software behavior, not physical battery characteristics.**

## Architecture

This simulation feeds software traces into the session and estimator code without requiring physical charge or discharge cycles.

* `trace.c/h`: A pseudo-random generator and trace constructors.
* `simulator.c/h`: Replay loop bridging traces into the production event/session loops.
* `test_simulation.c`: Replay test execution covering 10 extreme scenarios (including pathologics, temperature excursions, and 10,000-session scale constraints).
