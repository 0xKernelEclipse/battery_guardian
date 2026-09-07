#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "estimator.h"
#include "confidence.h"
#include "degradation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ModelFresh = 0,
    ModelAging,
    ModelStale,
    ModelInsufficientData
} ModelState;

typedef struct {
    float estimated_capacity_mah;
    float reference_capacity_mah;
    uint8_t observed_health_pct;

    uint8_t gauge_health_pct;

    BatteryConfidence confidence;
    BatteryDegradation degradation;

    uint32_t accepted_sessions;
    uint32_t rejected_sessions;

    uint64_t last_estimate_timestamp;

    bool estimate_available;
    bool trend_available;
    ModelState model_state;
} BatteryHealthSnapshot;

// Used for writing to journal
typedef struct __attribute__((packed)) {
    float estimated_capacity;
    float reference_capacity;
    uint8_t observed_health;
    uint8_t confidence;
    uint32_t accepted_sessions;
    uint32_t rejected_sessions;
    int8_t trend; // -1 declining, 0 stable, 1 improving
    uint8_t trend_confidence;
} EstimateRecordPayload;

// Full struct exposed here so tests can inspect internal estimator state.
// All GUI code must use the snapshot API and must not directly mutate fields.
typedef struct BatteryHealthEngine {
    CapacityEstimator estimator;
    BatteryConfidence confidence;
    BatteryDegradation degradation;

    float reference_capacity_mah;
    uint8_t gauge_health_pct;
    uint64_t last_timestamp;
    bool has_estimate;
} BatteryHealthEngine;

BatteryHealthEngine* battery_health_alloc(void);
void battery_health_free(BatteryHealthEngine* engine);

void battery_health_set_reference(BatteryHealthEngine* engine, float reference_mah);
void battery_health_set_gauge_health(BatteryHealthEngine* engine, uint8_t health_pct);
bool battery_health_process_session(BatteryHealthEngine* engine, const BatterySession* session, EstimateRecordPayload* out_payload);

void battery_health_load_estimate(BatteryHealthEngine* engine, const EstimateRecordPayload* payload, uint64_t timestamp);

void battery_health_get_snapshot(BatteryHealthEngine* engine, uint64_t current_timestamp, BatteryHealthSnapshot* snapshot);

#ifdef __cplusplus
}
#endif
