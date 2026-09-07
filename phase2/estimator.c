#include "estimator.h"
#include <string.h>
#include <stdlib.h>

void estimator_init(CapacityEstimator* est) {
    memset(est, 0, sizeof(CapacityEstimator));
}

static float calculate_median(const CapacityEstimator* est) {
    if(est->accepted_count == 0) return 0.0f;
    float values[ESTIMATOR_MAX_HISTORY];
    for(uint32_t i=0; i<est->accepted_count; i++) {
        values[i] = est->accepted[i].candidate_capacity_mah;
    }
    // Use insertion sort instead of qsort since qsort is disabled in Flipper API
    for(uint32_t i = 1; i < est->accepted_count; i++) {
        float key = values[i];
        int32_t j = i - 1;
        while(j >= 0 && values[j] > key) {
            values[j + 1] = values[j];
            j--;
        }
        values[j + 1] = key;
    }
    if(est->accepted_count % 2 == 0) {
        return (values[est->accepted_count/2 - 1] + values[est->accepted_count/2]) / 2.0f;
    } else {
        return values[est->accepted_count/2];
    }
}

CandidateRejectionReason estimator_process_session(CapacityEstimator* est, const BatterySession* session, CapacityCandidate* out_candidate) {
    if(!est || !session || !out_candidate) {
        return CandidateRejectedInvalidSession;
    }
    memset(out_candidate, 0, sizeof(CapacityCandidate));
    
    if(session->type != SessionTypeDischarging) {
        return CandidateRejectedInvalidSession;
    }
    
    if(session->start_timestamp >= session->end_timestamp || session->sample_count < 2) {
        return CandidateRejectedInvalidSession;
    }
    
    // Check quality flags
    if(!(session->quality_flags & SESSION_QUALITY_ENOUGH_SAMPLES) ||
       !(session->quality_flags & SESSION_QUALITY_MEANINGFUL_SOC) ||
       !(session->quality_flags & SESSION_QUALITY_NOT_INTERRUPTED) ||
       !(session->quality_flags & SESSION_QUALITY_NO_SENSOR_FAULT) ||
       !(session->quality_flags & SESSION_QUALITY_NO_TEMP_EXCURSION)) {
        return CandidateRejectedNoisyData;
    }
    
    uint8_t soc_drop = (session->start_soc > session->end_soc) ? (session->start_soc - session->end_soc) : 0;
    if(soc_drop < ESTIMATOR_MIN_SOC_DROP) {
        return CandidateRejectedInsufficientDrop;
    }
    
    float duration_hours = (session->end_timestamp - session->start_timestamp) / 3600000.0f;
    if(duration_hours <= 0.0f || duration_hours > 1000.0f) {
        return CandidateRejectedNoisyData; // Adversarial checks
    }
    
    // Calculate candidate capacity
    // Rule: We trust current integration. If the user discharged 100mAh and SOC went down 10%, 
    // total capacity = 100mAh / 0.1 = 1000mAh.
    
    float integrated_mah = session->accumulated_energy_mah;
    if (integrated_mah <= 0.0f) {
        return CandidateRejectedNoisyData; // Current must be negative/draining
    }
    
    float full_capacity_estimate = integrated_mah / (soc_drop / 100.0f);
    
    out_candidate->candidate_capacity_mah = full_capacity_estimate;
    out_candidate->session_id = session->session_id;
    out_candidate->evidence_type = CapacityEvidenceCurrentIntegration;
    out_candidate->start_soc = session->start_soc;
    out_candidate->end_soc = session->end_soc;
    out_candidate->duration_hours = duration_hours;
    out_candidate->avg_temperature = session->average_temperature;
    out_candidate->quality_flags = session->quality_flags;
    
    // Check if outlier
    if(est->accepted_count >= 3) {
        float median = calculate_median(est);
        if(median > 0.0f) {
            float diff = (full_capacity_estimate > median) ? (full_capacity_estimate - median) : (median - full_capacity_estimate);
            float pct_diff = (diff / median) * 100.0f;
            if(pct_diff > ESTIMATOR_OUTLIER_THRESHOLD_PCT) {
                est->total_rejected++;
                return CandidateRejectedOutlier;
            }
        }
    }
    
    // Accept candidate
    est->accepted[est->head] = *out_candidate;
    est->head = (est->head + 1) % ESTIMATOR_MAX_HISTORY;
    if(est->accepted_count < ESTIMATOR_MAX_HISTORY) {
        est->accepted_count++;
    }
    
    est->robust_estimate_mah = calculate_median(est);
    est->has_valid_estimate = true;
    est->total_accepted++;
    
    return CandidateRejectedNone;
}
