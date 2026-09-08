# Architecture

Battery Guardian is a C application with one worker thread. The worker reads telemetry, updates the session state, queues journal samples, evaluates the charge policy, and publishes a snapshot for the GUI.

```text
telemetry_adapter.c
  |
  +--> session.c --> phase2/estimator.c --> battery_health.c
  |                                      \-> confidence/degradation
  +--> storage/journal.c
  +--> phase2/charge_policy.c --> core/charger_hal.c
  |
  \--> model/battery_model.c --> gui/
```

## Telemetry

`core/telemetry_adapter.c` reads the Flipper power HAL and converts readings to volts, amps, Celsius, percent, and a monotonic millisecond timestamp. It records validity flags when a reading is unavailable.

## Sessions and estimates

`core/session.c` groups samples into charging, discharging, or idle sessions. A discharge session needs more than 10 samples and a 10-point SOC change to receive the session quality flag. The estimator applies the stricter 15-point SOC requirement before accepting a capacity candidate. It integrates current over the session, divides by the SOC change, rejects large outliers, and keeps a 16-entry history.

`phase2/battery_health.c` keeps the fuel gauge health value separately from the observed capacity estimate.

## Policy and charger interface

`phase2/charge_policy.c` evaluates the selected target and hysteresis rules. The host mock accepts suppression requests for tests. The production implementation in `core/charger_hal.c` reports no charge-control capability and rejects those requests, so policy state is not physical charge control.

## Storage

`storage/journal.c` writes a header followed by typed records and a CRC for each payload. It recovers the valid prefix after a partial write and caps the file at 512 KB. The journal mutex protects the file and sample ring buffer. File I/O happens while that mutex is held.

The application uses `/ext/apps_data/battery_guardian.log` through `EXT_PATH()`.

## Model and UI

`model/battery_model.c` owns the worker thread and copies a complete snapshot while holding its mutex. GUI code reads that snapshot and does not call storage or hardware APIs directly.
