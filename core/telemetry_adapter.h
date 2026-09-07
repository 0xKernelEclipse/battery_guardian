#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Capability flags indicating what the underlying platform can provide
#define TELEMETRY_CAP_NONE              (0)
#define TELEMETRY_CAP_VOLTAGE           (1 << 0)
#define TELEMETRY_CAP_CURRENT           (1 << 1)
#define TELEMETRY_CAP_TEMPERATURE       (1 << 2)
#define TELEMETRY_CAP_SOC               (1 << 3)
#define TELEMETRY_CAP_REMAINING_CAP     (1 << 4)
#define TELEMETRY_CAP_FULL_CAP          (1 << 5)
#define TELEMETRY_CAP_DESIGN_CAP        (1 << 6)
#define TELEMETRY_CAP_HEALTH_PCT        (1 << 7)
#define TELEMETRY_CAP_USB_VOLTAGE       (1 << 8)
#define TELEMETRY_CAP_CHARGING_STATE    (1 << 9)
#define TELEMETRY_CAP_GAUGE_STATUS      (1 << 10)

// Standard capability mask supported by Flipper Zero F7 hardware
#define TELEMETRY_CAP_FLIPPER_F7 ( \
    TELEMETRY_CAP_VOLTAGE | \
    TELEMETRY_CAP_CURRENT | \
    TELEMETRY_CAP_TEMPERATURE | \
    TELEMETRY_CAP_SOC | \
    TELEMETRY_CAP_REMAINING_CAP | \
    TELEMETRY_CAP_FULL_CAP | \
    TELEMETRY_CAP_DESIGN_CAP | \
    TELEMETRY_CAP_HEALTH_PCT | \
    TELEMETRY_CAP_USB_VOLTAGE | \
    TELEMETRY_CAP_CHARGING_STATE | \
    TELEMETRY_CAP_GAUGE_STATUS )

// Telemetry health/operational state
typedef enum {
    TelemetryStateNormal = 0,    // All primary metrics valid and in safe ranges
    TelemetryStatePartial,       // Non-critical metrics missing (e.g. current or gauge health)
    TelemetryStateFault,         // Sensor failure, hostile value, or gauge communication broken
    TelemetryStateUnavailable,   // Power subsystem completely unreachable
} TelemetryState;

// Normalized physical SI telemetry data
typedef struct {
    uint64_t timestamp_ms;          // Monotonic elapsed time in milliseconds
    float voltage_v;                // Pack voltage in Volts (e.g. 3.82f)
    float current_a;                // Pack current in Amperes (+ = charge, - = discharge)
    float temperature_c;            // Cell temperature in Celsius (e.g. 24.5f)
    uint8_t soc_pct;                // State of charge (0 - 100 %)
    uint32_t remaining_capacity_mah;// Remaining capacity in milliampere-hours
    uint32_t full_capacity_mah;     // Full charge capacity in milliampere-hours
    uint32_t design_capacity_mah;   // Factory design capacity in milliampere-hours
    uint8_t gauge_health_pct;       // Fuel gauge reported SOH (0 - 100 %)
    float charge_voltage_limit_v;   // Active PMIC charge limit in Volts
    float usb_voltage_v;            // VBUS input voltage in Volts
    bool charging;                  // True if PMIC indicates active charging
    bool charging_done;             // True if charge cycle completed
    bool gauge_ok;                  // True if fuel gauge self-test passed
    bool usb_present;               // True if VBUS detected (>4.0V) or charging
    uint32_t validity_flags;        // Bitmask of validated fields (VALID_* flags)
    uint32_t capabilities;          // Bitmask of supported hardware capabilities
    TelemetryState state;           // Overall telemetry operational state
} TelemetryRawSample;

// Platform Telemetry Adapter Interface
typedef struct {
    bool (*read)(TelemetryRawSample* out_sample, void* context);
    uint32_t (*get_capabilities)(void* context);
    TelemetryState (*get_state)(void* context);
} TelemetryAdapterInterface;

// Initialize the telemetry adapter subsystem
void telemetry_adapter_init(void);

// Read a normalized, validated sample from the underlying platform
bool telemetry_adapter_read_sample(TelemetryRawSample* out_sample);

// Get current hardware capabilities bitmask
uint32_t telemetry_adapter_get_capabilities(void);

// Get current operational telemetry state
TelemetryState telemetry_adapter_get_state(void);

// For testing / simulated environments: set custom backend
void telemetry_adapter_set_backend(const TelemetryAdapterInterface* iface, void* context);

#ifdef __cplusplus
}
#endif
