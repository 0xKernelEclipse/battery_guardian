# Hardware testing

Physical testing has not been completed. The production charger HAL is passive, so the current FAP only observes readings and evaluates policy state. It does not write PMIC charge-control registers.

Do not enable active control on a battery until the following checks have been run on a sacrificial test setup and recorded:

1. Compare voltage, current, temperature, and SOC readings with calibrated instruments across the normal battery range.
2. Disconnect and reconnect USB while the app is logging. Check the policy state and journal after contact bounce and abrupt removal.
3. Interrupt the fuel-gauge I2C lines and confirm that the app reports a sensor fault without locking up the Flipper.
4. Run controlled discharge sessions with a battery simulator and compare the observed capacity estimate with the simulator's measured capacity.
5. Test temperature limits and recovery while monitoring the cell with an external sensor.
6. Only after the passive checks pass, test charge suppression with an oscilloscope, current measurement, and I2C capture.

Record the device revision, firmware version, test build, instruments, raw readings, and failures. This repository currently contains no physical test results.
