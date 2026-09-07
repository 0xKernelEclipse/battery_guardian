#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../core/telemetry.h"

// Deterministic PRNG seed initialization
void sim_prng_seed(uint32_t seed);

// Get next deterministic random uint32
uint32_t sim_prng_next(void);

// Get a random float between 0.0 and 1.0
float sim_prng_next_float(void);

// Get a random float between min and max
float sim_prng_range(float min, float max);

// Compact synthetic telemetry sample definition
typedef struct {
    uint64_t timestamp_ms;
    float voltage_v;
    float current_a;
    float temperature_c;
    uint8_t soc_pct;
    uint32_t remaining_capacity_mah;
    uint32_t full_capacity_mah;
    uint32_t design_capacity_mah;
    uint8_t gauge_health_pct;
    bool gauge_ok;
    bool charging;
    bool usb_present;
    uint32_t flags;
} SimulatedTelemetry;

// Convert SimulatedTelemetry to production BatteryTelemetry
void trace_to_telemetry(const SimulatedTelemetry* sim, BatteryTelemetry* out_tel);

// Generate a series of basic simulated samples for a discharge curve
void trace_generate_discharge_curve(
    SimulatedTelemetry* buffer, 
    size_t count, 
    uint64_t start_time, 
    uint8_t start_soc, 
    uint8_t end_soc, 
    float capacity_mah,
    float noise_level);

// Generate a complete discharge session with an idle sample appended to properly terminate it.
// buffer must have space for count + 1 samples.
// returns count + 1
size_t trace_generate_discharge_session(
    SimulatedTelemetry* buffer, 
    size_t count, 
    uint64_t start_time, 
    uint8_t start_soc, 
    uint8_t end_soc, 
    float capacity_mah,
    float noise_level);

