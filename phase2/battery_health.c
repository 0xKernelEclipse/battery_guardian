#include "battery_health.h"
#include <stdlib.h>
#include <string.h>

BatteryHealthEngine* battery_health_alloc(void) {
    BatteryHealthEngine* engine = malloc(sizeof(BatteryHealthEngine));
    memset(engine, 0, sizeof(BatteryHealthEngine));
    engine->reference_capacity_mah = 2100.0f; // Default Flipper Zero design capacity
    estimator_init(&engine->estimator);
    return engine;
}

void battery_health_free(BatteryHealthEngine* engine) {
    free(engine);
}

void battery_health_set_reference(BatteryHealthEngine* engine, float reference_mah) {
    engine->reference_capacity_mah = reference_mah;
}

void battery_health_set_gauge_health(BatteryHealthEngine* engine, uint8_t health_pct) {
    engine->gauge_health_pct = health_pct;
}

bool battery_health_process_session(BatteryHealthEngine* engine, const BatterySession* session, EstimateRecordPayload* out_payload) {
    CapacityCandidate candidate;
    CandidateRejectionReason reason = estimator_process_session(&engine->estimator, session, &candidate);
    
    if (reason == CandidateRejectedNone) {
        engine->last_timestamp = session->end_timestamp;
        
        confidence_calculate(&engine->confidence, &engine->estimator);
        degradation_calculate(&engine->degradation, &engine->estimator, &engine->confidence);
        
        engine->has_estimate = engine->estimator.has_valid_estimate;
        
        if (engine->has_estimate && out_payload) {
            memset(out_payload, 0, sizeof(EstimateRecordPayload));
            out_payload->estimated_capacity = engine->estimator.robust_estimate_mah;
            out_payload->reference_capacity = engine->reference_capacity_mah;
            
            float health = (out_payload->estimated_capacity / out_payload->reference_capacity) * 100.0f;
            if(health > 100.0f) health = 100.0f; // Clamp
            out_payload->observed_health = (uint8_t)health;
            
            out_payload->confidence = engine->confidence.overall;
            out_payload->accepted_sessions = engine->estimator.total_accepted;
            out_payload->rejected_sessions = engine->estimator.total_rejected;
            
            if (engine->degradation.trend == DegradationTrendDeclining) out_payload->trend = -1;
            else if (engine->degradation.trend == DegradationTrendImproving) out_payload->trend = 1;
            else out_payload->trend = 0;
            
            out_payload->trend_confidence = engine->degradation.trend_confidence;
            return true;
        }
    }
    return false; // No new estimate was saved.
}

void battery_health_load_estimate(BatteryHealthEngine* engine, const EstimateRecordPayload* payload, uint64_t timestamp) {
    if(!engine || !payload) return;

    engine->last_timestamp = timestamp;
    engine->estimator.robust_estimate_mah = payload->estimated_capacity;
    engine->estimator.has_valid_estimate = payload->estimated_capacity > 0.0f;
    engine->has_estimate = engine->estimator.has_valid_estimate;

    if (payload->reference_capacity > 0.0f) {
        engine->reference_capacity_mah = payload->reference_capacity;
    }

    engine->confidence.overall = (ConfidenceLevel)payload->confidence;
    engine->estimator.total_accepted = payload->accepted_sessions;
    engine->estimator.total_rejected = payload->rejected_sessions;
    engine->degradation.trend_confidence = payload->trend_confidence;
    if (payload->trend < 0) {
        engine->degradation.trend = DegradationTrendDeclining;
    } else if (payload->trend > 0) {
        engine->degradation.trend = DegradationTrendImproving;
    } else {
        engine->degradation.trend = DegradationTrendStable;
    }
}

void battery_health_get_snapshot(BatteryHealthEngine* engine, uint64_t current_timestamp, BatteryHealthSnapshot* snapshot) {
    memset(snapshot, 0, sizeof(BatteryHealthSnapshot));
    snapshot->estimate_available = engine->has_estimate;
    
    if (!engine->has_estimate) {
        snapshot->model_state = ModelInsufficientData;
        return;
    }
    
    snapshot->estimated_capacity_mah = engine->estimator.robust_estimate_mah;
    snapshot->reference_capacity_mah = engine->reference_capacity_mah;
    
    float health = 0.0f;
    if (snapshot->reference_capacity_mah > 0.0f) {
        health = (snapshot->estimated_capacity_mah / snapshot->reference_capacity_mah) * 100.0f;
        if(health < 0.0f) health = 0.0f;
        if(health > 150.0f) health = 150.0f;
    }
    snapshot->observed_health_pct = (uint8_t)health;
    
    snapshot->gauge_health_pct = engine->gauge_health_pct;
    snapshot->confidence = engine->confidence;
    snapshot->degradation = engine->degradation;
    
    snapshot->accepted_sessions = engine->estimator.total_accepted;
    snapshot->rejected_sessions = engine->estimator.total_rejected;
    snapshot->last_estimate_timestamp = engine->last_timestamp;
    snapshot->trend_available = (engine->degradation.trend != DegradationTrendInsufficient);
    
    // Stale check
    uint64_t age_ms = (current_timestamp > engine->last_timestamp) ? (current_timestamp - engine->last_timestamp) : 0;
    float age_days = age_ms / (1000.0f * 60.0f * 60.0f * 24.0f);
    
    if (age_days > 30.0f) {
        snapshot->model_state = ModelStale;
    } else if (age_days > 14.0f) {
        snapshot->model_state = ModelAging;
    } else {
        snapshot->model_state = ModelFresh;
    }
}
