#include "simulator.h"
#include <string.h>

void simulator_init(void) {
    // Reset global state for tests if needed, although session_manager_init resets session logic
}

bool simulation_replay_trace(
    const SimulatedTelemetry* samples,
    size_t count,
    SimulationResult* result) 
{
    memset(result, 0, sizeof(SimulationResult));
    if (count == 0 || samples == NULL) {
        return false;
    }
    
    // 1. Initialize production modules
    session_manager_init();
    BatteryHealthEngine* engine = battery_health_alloc();
    if (!engine) {
        result->crashed = true;
        return false;
    }
    
    uint32_t last_processed_session = 0;
    BatterySession completed;
    
    // Clear any lingering session state from previous test
    if (session_get_last_completed(&completed)) {
        last_processed_session = completed.session_id; 
    }
    
    // 2. Pump telemetry through the session state machine
    for (size_t i = 0; i < count; i++) {
        BatteryTelemetry sample;
        trace_to_telemetry(&samples[i], &sample);
        
        // Pass sample to production session logic
        session_process_sample(&sample);
        
        // Did it produce a completed session?
        if (session_get_last_completed(&completed)) {
            if (completed.session_id != last_processed_session) {
                // Pass completed session to production capacity logic
                EstimateRecordPayload out_payload;
                battery_health_process_session(engine, &completed, &out_payload);
                last_processed_session = completed.session_id;
                result->total_sessions_processed++;
            }
        }
    }
    
    // Force end the last session by sending an idle sample way in the future
    // so the state machine flushes it out.
    BatteryTelemetry flush_sample;
    trace_to_telemetry(&samples[count - 1], &flush_sample);
    flush_sample.timestamp_ms += 10000;
    flush_sample.current_a = 0.0f;
    flush_sample.charging = false;
    session_process_sample(&flush_sample);
    
    if (session_get_last_completed(&completed)) {
        if (completed.session_id != last_processed_session) {
            EstimateRecordPayload out_payload;
            battery_health_process_session(engine, &completed, &out_payload);
            last_processed_session = completed.session_id;
            result->total_sessions_processed++;
        }
    }
    
    // 3. Gather output snapshot
    battery_health_get_snapshot(engine, samples[count-1].timestamp_ms, &result->snapshot);
    
    // Gather deep diagnostics
    result->total_candidates_accepted = engine->estimator.total_accepted;
    result->total_candidates_rejected = engine->estimator.total_rejected;
    result->robust_estimate_mah = engine->estimator.robust_estimate_mah;
    
    battery_health_free(engine);
    return true;
}

void trace_generate_discharge_curve(
    SimulatedTelemetry* buffer, 
    size_t count, 
    uint64_t start_time, 
    uint8_t start_soc, 
    uint8_t end_soc, 
    float capacity_mah,
    float noise_level) 
{
    if (count < 2) return;
    
    float soc_drop = (float)(start_soc - end_soc);
    if(soc_drop <= 0) soc_drop = 1.0f;
    
    // Average current needed to drain (soc_drop / 100) of capacity_mah over the time period
    // But wait, time period is determined by samples. Let's make it 1 sample = 1 minute (60000 ms)
    uint64_t dt_ms = 60000; 
    float total_hours = (count * dt_ms) / 3600000.0f;
    
    float total_mah_drained = capacity_mah * (soc_drop / 100.0f);
    float avg_current_ma = total_mah_drained / total_hours;
    float avg_current_a = avg_current_ma / 1000.0f;
    
    for (size_t i = 0; i < count; i++) {
        float progress = (float)i / (float)(count - 1);
        
        buffer[i].timestamp_ms = start_time + (i * dt_ms);
        
        // Voltage goes down linearly as an approximation
        buffer[i].voltage_v = 4.1f - (1.1f * progress);
        
        // Add noise to current
        float current_noise = sim_prng_range(-noise_level, noise_level);
        buffer[i].current_a = -(avg_current_a + current_noise);
        
        // Temperature stable around 25C with noise
        buffer[i].temperature_c = 25.0f + sim_prng_range(-1.0f, 1.0f);
        
        // SOC drops linearly
        buffer[i].soc_pct = start_soc - (uint8_t)(progress * soc_drop);
        
        buffer[i].remaining_capacity_mah = capacity_mah * (buffer[i].soc_pct / 100.0f);
        buffer[i].full_capacity_mah = capacity_mah;
        buffer[i].design_capacity_mah = 2100;
        buffer[i].gauge_health_pct = (uint8_t)((capacity_mah / 2100.0f) * 100.0f);
        
        buffer[i].gauge_ok = true;
        buffer[i].charging = false;
        buffer[i].flags = 0xFFFFFFFF; // all valid
    }
}

size_t trace_generate_discharge_session(
    SimulatedTelemetry* buffer, 
    size_t count, 
    uint64_t start_time, 
    uint8_t start_soc, 
    uint8_t end_soc, 
    float capacity_mah,
    float noise_level)
{
    if (count == 0) return 0;
    
    trace_generate_discharge_curve(buffer, count, start_time, start_soc, end_soc, capacity_mah, noise_level);
    
    // Add idle sample
    buffer[count] = buffer[count - 1];
    buffer[count].timestamp_ms += 60000;
    buffer[count].current_a = 0.0f;
    buffer[count].charging = false;
    
    return count + 1;
}
