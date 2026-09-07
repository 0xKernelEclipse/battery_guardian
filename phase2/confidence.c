#include "confidence.h"
#include <stdio.h>
#include <string.h>

void confidence_calculate(BatteryConfidence* conf, const CapacityEstimator* est) {
    if(!conf || !est) return;
    memset(conf, 0, sizeof(BatteryConfidence));
    conf->evidence_sessions = est->total_accepted + est->total_rejected;
    conf->accepted_sessions = est->total_accepted;
    conf->rejected_sessions = est->total_rejected;
    
    if (est->accepted_count < 3) {
        conf->capacity_confidence = ConfidenceLevelInsufficient;
        conf->overall = ConfidenceLevelInsufficient;
        snprintf(conf->explanation, sizeof(conf->explanation), "%lu qualifying sessions. Insufficient data.", (unsigned long)est->accepted_count);
        return;
    }
    
    // Variance check
    float min_c = 99999.0f;
    float max_c = 0.0f;
    for(uint32_t i=0; i<est->accepted_count; i++) {
        float c = est->accepted[i].candidate_capacity_mah;
        if (c < min_c) min_c = c;
        if (c > max_c) max_c = c;
    }
    float spread_pct = 0.0f;
    if (est->robust_estimate_mah > 0) {
        spread_pct = ((max_c - min_c) / est->robust_estimate_mah) * 100.0f;
    }
    
    if (spread_pct < 5.0f && est->accepted_count >= 5) {
        conf->capacity_confidence = ConfidenceLevelHigh;
    } else if (spread_pct < 10.0f && est->accepted_count >= 3) {
        conf->capacity_confidence = ConfidenceLevelMedium;
    } else {
        conf->capacity_confidence = ConfidenceLevelLow;
    }
    
    if (est->total_rejected > est->total_accepted) {
        conf->data_quality = ConfidenceLevelLow;
    } else if (est->total_rejected > 0) {
        conf->data_quality = ConfidenceLevelMedium;
    } else {
        conf->data_quality = ConfidenceLevelHigh;
    }
    
    if (conf->capacity_confidence == ConfidenceLevelHigh && conf->data_quality >= ConfidenceLevelMedium) {
        conf->overall = ConfidenceLevelHigh;
    } else if (conf->capacity_confidence >= ConfidenceLevelMedium && conf->data_quality >= ConfidenceLevelMedium) {
        conf->overall = ConfidenceLevelMedium;
    } else {
        conf->overall = ConfidenceLevelLow;
    }
    
    snprintf(conf->explanation, sizeof(conf->explanation), 
             "%lu consistent estimates.\n%lu rejected outliers.\n%s spread.", 
             (unsigned long)est->accepted_count, (unsigned long)est->total_rejected,
             (spread_pct < 5.0f) ? "Low" : ((spread_pct < 10.0f) ? "Medium" : "High"));
}
