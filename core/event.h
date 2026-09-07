#pragma once

#include "telemetry.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EventSeverityInfo = 0,
    EventSeverityWarning,
    EventSeverityError,
    EventSeverityCritical
} EventSeverity;

typedef enum {
    EventTypeChargeStarted,
    EventTypeChargeEnded,
    EventTypeChargeInterrupted,
    EventTypeDischargeStarted,
    EventTypeLowBattery,
    EventTypeCriticalBattery,
    EventTypeUsbConnected,
    EventTypeUsbDisconnected,
    EventTypeTemperatureSpike,
    EventTypeTemperatureRecovery,
    EventTypeVoltageAnomaly,
    EventTypeCurrentAnomaly,
    EventTypeGaugeError,
    EventTypeSocInconsistency,
    EventTypeCapacityEstimateUpdated,
    EventTypeCapacityEstimateRejected
} EventType;

typedef struct {
    uint64_t timestamp_ms;
    EventType type;
    EventSeverity severity;
    char short_explanation[32]; // e.g. "29.2C -> 35.8C"
    BatteryTelemetry snapshot;
} BatteryEvent;

typedef void (*EventListenerCallback)(const BatteryEvent* event, void* context);

// Register event listener
void event_set_listener(EventListenerCallback callback, void* context);

// Generate an event, optionally writing it to the journal
void event_generate(EventType type, EventSeverity severity, const char* explanation, const BatteryTelemetry* snapshot);

// Get last generated event
bool event_get_last(BatteryEvent* out_event);

// Reset event state (for tests)
void event_reset_state(void);

#ifdef __cplusplus
}
#endif
