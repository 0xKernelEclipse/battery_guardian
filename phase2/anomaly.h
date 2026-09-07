#pragma once

#include "telemetry.h"
#include "session.h"
#include "estimator.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AnomalyNone = 0,
    AnomalyVoltageDrop = (1 << 0),
    AnomalyTemperatureSpike = (1 << 1),
    AnomalyCapacityCollapse = (1 << 2),
    AnomalyGaugeError = (1 << 3),
} AnomalyFlags;

// Initialize anomaly detection engine
void anomaly_init(void);

// Check sample for instantaneous anomalies
AnomalyFlags anomaly_check_sample(const BatteryTelemetrySample* sample);

// Check session for longer-term anomalies
AnomalyFlags anomaly_check_session(const BatterySession* session, const BatteryProfile* profile);

#ifdef __cplusplus
}
#endif
