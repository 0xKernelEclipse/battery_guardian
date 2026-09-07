#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "trace.h"
#include "../../phase2/battery_health.h"
#include "../../core/session.h"

// The result of a simulation replay
typedef struct {
    BatteryHealthSnapshot snapshot;
    uint32_t total_sessions_processed;
    uint32_t total_candidates_accepted;
    uint32_t total_candidates_rejected;
    float robust_estimate_mah;
    bool crashed;
} SimulationResult;

// Initialize the simulator (must be called before replay)
void simulator_init(void);

// Replay a trace through the production logic
bool simulation_replay_trace(
    const SimulatedTelemetry* samples,
    size_t count,
    SimulationResult* result);

