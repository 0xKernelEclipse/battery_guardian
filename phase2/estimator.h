#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../core/session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CapacityEvidenceNone = 0,
    CapacityEvidenceRemainingDelta,
    CapacityEvidenceCurrentIntegration,
} CapacityEvidenceType;

typedef struct {
    float candidate_capacity_mah;
    uint32_t session_id;
    CapacityEvidenceType evidence_type;
    uint8_t start_soc;
    uint8_t end_soc;
    float duration_hours;
    float avg_temperature;
    uint32_t quality_flags;
} CapacityCandidate;

typedef enum {
    CandidateRejectedNone = 0,
    CandidateRejectedInvalidSession,
    CandidateRejectedInsufficientDrop,
    CandidateRejectedNoisyData,
    CandidateRejectedOutlier
} CandidateRejectionReason;

// Configuration constants exposed for testing
#define ESTIMATOR_MIN_SOC_DROP 15
#define ESTIMATOR_MAX_HISTORY 16
#define ESTIMATOR_OUTLIER_THRESHOLD_PCT 15.0f

typedef struct {
    CapacityCandidate accepted[ESTIMATOR_MAX_HISTORY];
    uint32_t accepted_count;
    uint32_t head;
    
    // Median of the accepted candidates.
    float robust_estimate_mah;
    bool has_valid_estimate;
    
    // Stats
    uint32_t total_accepted;
    uint32_t total_rejected;
} CapacityEstimator;

void estimator_init(CapacityEstimator* est);
CandidateRejectionReason estimator_process_session(CapacityEstimator* est, const BatterySession* session, CapacityCandidate* out_candidate);

#ifdef __cplusplus
}
#endif
