#pragma once

#include "telemetry.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SessionTypeUnknown = 0,
    SessionTypeCharging,
    SessionTypeDischarging,
    SessionTypeIdle
} SessionType;

// Quality flags for estimation acceptance
#define SESSION_QUALITY_ENOUGH_SAMPLES      (1 << 0)
#define SESSION_QUALITY_GOOD_SPACING        (1 << 1)
#define SESSION_QUALITY_STABLE_TELEMETRY    (1 << 2)
#define SESSION_QUALITY_MEANINGFUL_SOC      (1 << 3)
#define SESSION_QUALITY_NO_SENSOR_FAULT     (1 << 4)
#define SESSION_QUALITY_NO_TEMP_EXCURSION   (1 << 5)
#define SESSION_QUALITY_NOT_INTERRUPTED     (1 << 6)

typedef struct {
    uint32_t session_id;
    SessionType type;
    
    uint64_t start_timestamp;
    uint64_t end_timestamp;
    
    uint8_t start_soc;
    uint8_t end_soc;
    
    float start_voltage;
    float end_voltage;
    
    float min_voltage;
    float max_voltage;
    
    float min_temperature;
    float max_temperature;
    float average_temperature;
    
    float min_current;
    float max_current;
    float average_current;
    
    float accumulated_energy_mah; // Energy-like accumulated current integration
    
    uint32_t sample_count;
    uint32_t quality_flags;
    
    bool is_active;
} BatterySession;

// Initialize session tracking
void session_manager_init(void);

// Process a new telemetry sample and update session state
void session_process_sample(const BatteryTelemetry* sample);

// Get the current active session
bool session_get_active(BatterySession* out_session);

// Get the last completed session
bool session_get_last_completed(BatterySession* out_session);

// End current session explicitly
void session_end_current(const BatteryTelemetry* sample);

#ifdef __cplusplus
}
#endif
