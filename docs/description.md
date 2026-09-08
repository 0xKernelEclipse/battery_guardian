Battery Guardian records battery telemetry while it is running on a Flipper Zero. It tracks charge and discharge sessions, estimates capacity from discharge current and SOC change, stores history on the SD card, and shows the estimate alongside the fuel gauge health value.

The production charger interface is passive. The app evaluates charge policies but does not write PMIC charge-control registers. Physical hardware testing has not been completed.
