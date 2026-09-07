#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "estimator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ConfidenceLevelInsufficient = 0,
    ConfidenceLevelLow,
    ConfidenceLevelMedium,
    ConfidenceLevelHigh
} ConfidenceLevel;

typedef struct {
    ConfidenceLevel overall;
    ConfidenceLevel data_quality;
    ConfidenceLevel capacity_confidence;
    ConfidenceLevel trend_confidence;

    uint16_t evidence_sessions;
    uint16_t accepted_sessions;
    uint16_t rejected_sessions;
    
    char explanation[128]; // e.g. "4 consistent candidates, 1 rejected outlier"
} BatteryConfidence;

void confidence_calculate(BatteryConfidence* conf, const CapacityEstimator* est);

#ifdef __cplusplus
}
#endif
