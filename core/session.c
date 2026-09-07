#include "session.h"
#include "event.h"
#include "../storage/journal.h"
#include <furi.h>
#include <string.h>

static BatterySession current_session;
static BatterySession last_completed_session;
static bool has_completed_session = false;
static uint32_t session_id_counter = 1; // Later load from baseline

// Accumulators for averages
static float temp_accumulator = 0.0f;
static float current_accumulator = 0.0f;
static uint64_t last_sample_timestamp = 0;
static float last_sample_current = 0.0f;
static bool has_previous_timestamp = false;


void session_manager_init(void) {
    memset(&current_session, 0, sizeof(BatterySession));
    current_session.is_active = false;
    has_previous_timestamp = false;
    last_sample_timestamp = 0;
    temp_accumulator = 0.0f;
    current_accumulator = 0.0f;
    session_id_counter = 1;
}

static void session_start(const BatteryTelemetry* sample, SessionType type) {
    memset(&current_session, 0, sizeof(BatterySession));
    current_session.session_id = session_id_counter++;
    current_session.type = type;
    
    current_session.start_timestamp = sample->timestamp_ms;
    current_session.start_soc = sample->soc_pct;
    current_session.start_voltage = sample->voltage_v;
    
    current_session.min_voltage = sample->voltage_v;
    current_session.max_voltage = sample->voltage_v;
    
    current_session.min_temperature = sample->temperature_c;
    current_session.max_temperature = sample->temperature_c;
    
    current_session.min_current = sample->current_a;
    current_session.max_current = sample->current_a;
    
    current_session.sample_count = 1;
    current_session.quality_flags = SESSION_QUALITY_NO_SENSOR_FAULT | 
                                    SESSION_QUALITY_NOT_INTERRUPTED | 
                                    SESSION_QUALITY_NO_TEMP_EXCURSION;
    
    temp_accumulator = sample->temperature_c;
    current_accumulator = sample->current_a;
    current_session.accumulated_energy_mah = 0.0f;
    last_sample_timestamp = sample->timestamp_ms;
    last_sample_current = sample->current_a;
    has_previous_timestamp = true;
    
    current_session.is_active = true;
    
    // Generate event
    EventType evt_type = EventTypeChargeStarted;
    if (type == SessionTypeDischarging) evt_type = EventTypeDischargeStarted;
    // We omit IDLE starts as events for brevity
    
    if (type == SessionTypeCharging || type == SessionTypeDischarging) {
        event_generate(evt_type, EventSeverityInfo, "Session started", sample);
    }
}

void session_process_sample(const BatteryTelemetry* sample) {
    if(!sample) return;
    SessionType required_type = SessionTypeIdle;
    
    if (sample->charging) {
        required_type = SessionTypeCharging;
    } else if (sample->current_a < -0.05f) {
        required_type = SessionTypeDischarging;
    }

    if (!current_session.is_active) {
        session_start(sample, required_type);
    } else {
        if (current_session.type != required_type) {
            session_end_current(sample);
            session_start(sample, required_type);
        } else {
            // Update running session
            current_session.end_timestamp = sample->timestamp_ms;
            current_session.end_soc = sample->soc_pct;
            current_session.end_voltage = sample->voltage_v;
            
            if (sample->voltage_v < current_session.min_voltage) current_session.min_voltage = sample->voltage_v;
            if (sample->voltage_v > current_session.max_voltage) current_session.max_voltage = sample->voltage_v;
            
            if (sample->temperature_c < current_session.min_temperature) current_session.min_temperature = sample->temperature_c;
            if (sample->temperature_c > current_session.max_temperature) current_session.max_temperature = sample->temperature_c;
            
            if (sample->current_a < current_session.min_current) current_session.min_current = sample->current_a;
            if (sample->current_a > current_session.max_current) current_session.max_current = sample->current_a;
            
            temp_accumulator += sample->temperature_c;
            current_accumulator += sample->current_a;
            current_session.sample_count++;
            
            current_session.average_temperature = temp_accumulator / current_session.sample_count;
            current_session.average_current = current_accumulator / current_session.sample_count;
            
            // Trapezoidal integration for energy (Amperes * hours * 1000 = mAh)
            if (has_previous_timestamp && sample->timestamp_ms > last_sample_timestamp) {
                float dt_hours = (sample->timestamp_ms - last_sample_timestamp) / 3600000.0f;
                float prev_curr = last_sample_current;
                float curr = sample->current_a;
                float abs_avg_curr = (prev_curr + curr) / 2.0f;
                if(abs_avg_curr < 0.0f) abs_avg_curr = -abs_avg_curr;
                current_session.accumulated_energy_mah += (abs_avg_curr * 1000.0f) * dt_hours;
            }
            last_sample_timestamp = sample->timestamp_ms;
            last_sample_current = sample->current_a;
            has_previous_timestamp = true;
            
            if (current_session.max_temperature > 45.0f) {
                current_session.quality_flags &= ~SESSION_QUALITY_NO_TEMP_EXCURSION;
            }
            if (!sample->gauge_ok) {
                current_session.quality_flags &= ~SESSION_QUALITY_NO_SENSOR_FAULT;
            }
        }
    }
}

void session_end_current(const BatteryTelemetry* sample) {
    if (current_session.is_active) {
        current_session.is_active = false;
        
        // Evaluate remaining quality flags
        if (current_session.sample_count > 10) current_session.quality_flags |= SESSION_QUALITY_ENOUGH_SAMPLES;
        
        uint8_t soc_diff = (current_session.start_soc > current_session.end_soc) ? 
                           (current_session.start_soc - current_session.end_soc) :
                           (current_session.end_soc - current_session.start_soc);
        if (soc_diff >= 10) {
            current_session.quality_flags |= SESSION_QUALITY_MEANINGFUL_SOC;
        }

        // Generate end events
        if (current_session.type == SessionTypeCharging) {
            event_generate(EventTypeChargeEnded, EventSeverityInfo, "Charge completed/stopped", sample);
        }
        
        uint64_t ts = sample ? sample->timestamp_ms : current_session.end_timestamp;
        // Write session to journal
        journal_write_record(RecordTypeSession, 1 /* SESSION_PAYLOAD_VERSION */, ts, &current_session, sizeof(BatterySession));
        
        last_completed_session = current_session;
        has_completed_session = true;
    }
}

bool session_get_active(BatterySession* out_session) {
    if (!current_session.is_active) return false;
    if (out_session) {
        memcpy(out_session, &current_session, sizeof(BatterySession));
    }
    return true;
}

bool session_get_last_completed(BatterySession* out_session) {
    if (!has_completed_session) return false;
    if (out_session) {
        memcpy(out_session, &last_completed_session, sizeof(BatterySession));
    }
    return true;
}
