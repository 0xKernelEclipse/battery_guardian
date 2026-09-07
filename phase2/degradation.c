#include "degradation.h"
#include <string.h>
#include <math.h>

void degradation_calculate(BatteryDegradation* deg, const CapacityEstimator* est, const BatteryConfidence* conf) {
    if(!deg || !est || !conf) return;
    memset(deg, 0, sizeof(BatteryDegradation));
    
    if (est->accepted_count < 5 || conf->overall == ConfidenceLevelInsufficient) {
        deg->trend = DegradationTrendInsufficient;
        deg->trend_confidence = ConfidenceLevelInsufficient;
        return;
    }
    
    // Simple linear regression to find slope (mAh per session step)
    // We assume the elements in est->accepted are somewhat ordered by time
    // est->head points to the next insertion. We must read them chronologically.
    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;
    uint32_t n = est->accepted_count;
    
    for(uint32_t i=0; i<n; i++) {
        uint32_t idx = (est->head + ESTIMATOR_MAX_HISTORY - n + i) % ESTIMATOR_MAX_HISTORY;
        float x = (float)i;
        float y = est->accepted[idx].candidate_capacity_mah;
        
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }
    
    float denominator = (n * sum_xx - sum_x * sum_x);
    if (fabsf(denominator) < 1e-6f) {
        deg->trend = DegradationTrendInsufficient;
        return;
    }
    
    float slope = (n * sum_xy - sum_x * sum_y) / denominator; // mAh per cycle
    deg->capacity_slope = slope;
    
    float base_cap = est->robust_estimate_mah;
    if (base_cap > 0.0f) {
        deg->percent_per_100_cycles = (slope * 100.0f / base_cap) * 100.0f;
    } else {
        deg->percent_per_100_cycles = 0.0f;
    }
    
    if (slope < -1.0f) {
        deg->trend = DegradationTrendDeclining;
    } else if (slope > 1.0f) {
        deg->trend = DegradationTrendImproving;
    } else {
        deg->trend = DegradationTrendStable;
    }
    
    if (conf->overall >= ConfidenceLevelHigh && n >= 10) {
        deg->trend_confidence = ConfidenceLevelHigh;
    } else if (conf->overall >= ConfidenceLevelMedium && n >= 5) {
        deg->trend_confidence = ConfidenceLevelMedium;
    } else {
        deg->trend_confidence = ConfidenceLevelLow;
    }
}
