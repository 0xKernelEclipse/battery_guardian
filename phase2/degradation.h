#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "estimator.h"
#include "confidence.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DegradationTrendInsufficient = 0,
    DegradationTrendStable,
    DegradationTrendDeclining,
    DegradationTrendImproving // Might be noisy
} DegradationTrendType;

typedef struct {
    float capacity_slope; // mAh per session
    float percent_per_100_cycles;
    float percent_per_30_days;

    ConfidenceLevel trend_confidence;

    DegradationTrendType trend;
} BatteryDegradation;

void degradation_calculate(BatteryDegradation* deg, const CapacityEstimator* est, const BatteryConfidence* conf);

#ifdef __cplusplus
}
#endif
