# Battery Guardian: Simulation Harness

## Hardware Boundary Disclaimer

**WARNING: THIS SIMULATION FRAMEWORK VALIDATES SOFTWARE LOGIC ONLY.**

The test vectors and simulated traces in this directory **DO NOT PROVE**:
* Real battery capacity
* Real charger behavior
* Real power-management IC (PMIC) characteristics
* Real gauge accuracy

Those elements **require physical hardware** to validate. The purpose of this simulation harness is exclusively to prove the mathematical and state-machine soundness of the **Capacity Learning Engine** algorithms against bounded synthetic data.

Explicitly: **Synthetic traces validate software behavior, not physical battery characteristics.**

## Architecture

This simulation feeds a deterministic software trace directly into the exact production firmware state machine (`core/session.c` -> `phase2/estimator.c`), running precisely the same code that will execute on the Flipper Zero MCU, but without requiring physical charge/discharge cycles.

* `trace.c/h`: A deterministic LCG Pseudo-Random Number Generator and trace constructors.
* `simulator.c/h`: Replay loop bridging traces into the production event/session loops.
* `test_simulation.c`: Replay test execution covering 10 extreme scenarios (including pathologics, temperature excursions, and 10,000-session scale constraints).
