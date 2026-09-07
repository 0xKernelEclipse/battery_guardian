#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Validity flags
#define VALID_VOLTAGE               (1 << 0)
#define VALID_CURRENT               (1 << 1)
#define VALID_TEMP                  (1 << 2)
#define VALID_SOC                   (1 << 3)
#define VALID_REMAINING_CAPACITY    (1 << 4)
#define VALID_FULL_CAPACITY         (1 << 5)
#define VALID_DESIGN_CAPACITY       (1 << 6)
#define VALID_HEALTH                (1 << 7)
#define VALID_GAUGE                 (1 << 8)

typedef struct {
    uint64_t timestamp_ms; // Monotonic timestamp
    float voltage_v;
    float current_a;
    float temperature_c;
    uint8_t soc_pct;
    uint32_t remaining_capacity_mah;
    uint32_t full_capacity_mah;
    uint32_t design_capacity_mah;
    uint8_t gauge_health_pct;
    float charge_voltage_limit_v;
    bool gauge_ok;
    bool usb_present;
    bool charging;
    uint32_t flags;
} BatteryTelemetry;

// Sampling Engine Configuration
typedef enum {
    SamplingModeIdle = 0,
    SamplingModeActive,
    SamplingModeCharging,
    SamplingModeDischarging,
    SamplingModeAnomaly
} SamplingMode;

// Initialize telemetry engine
void telemetry_init(void);

// Deinitialize telemetry engine
void telemetry_free(void);

// Read current hardware state and generate a validated sample
bool telemetry_take_sample(BatteryTelemetry* sample);

// Change adaptive sampling mode
void telemetry_set_sampling_mode(SamplingMode mode);

// Get recommended sampling interval (ms) for current mode
uint32_t telemetry_get_interval_ms(void);

#ifdef __cplusplus
}
#endif
