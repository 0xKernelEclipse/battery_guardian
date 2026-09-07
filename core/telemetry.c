#include "telemetry.h"
#include "telemetry_adapter.h"
#include <furi.h>
#include <string.h>

static SamplingMode current_mode = SamplingModeIdle;

void telemetry_init(void) {
    telemetry_adapter_init();
    current_mode = SamplingModeIdle;
}

void telemetry_free(void) {
    // Cleanup if needed
}

void telemetry_set_sampling_mode(SamplingMode mode) {
    current_mode = mode;
}

uint32_t telemetry_get_interval_ms(void) {
    switch (current_mode) {
        case SamplingModeIdle: return 30000;
        case SamplingModeActive: return 10000;
        case SamplingModeCharging: return 5000;
        case SamplingModeDischarging: return 10000;
        case SamplingModeAnomaly: return 1000;
        default: return 30000;
    }
}

bool telemetry_take_sample(BatteryTelemetry* sample) {
    if(!sample) return false;
    memset(sample, 0, sizeof(BatteryTelemetry));

    TelemetryRawSample raw;
    if (!telemetry_adapter_read_sample(&raw)) {
        return false;
    }

    sample->timestamp_ms = raw.timestamp_ms;
    sample->voltage_v = raw.voltage_v;
    sample->current_a = raw.current_a;
    sample->temperature_c = raw.temperature_c;
    sample->soc_pct = raw.soc_pct;
    sample->remaining_capacity_mah = raw.remaining_capacity_mah;
    sample->full_capacity_mah = raw.full_capacity_mah;
    sample->design_capacity_mah = raw.design_capacity_mah;
    sample->gauge_health_pct = raw.gauge_health_pct;
    sample->charge_voltage_limit_v = raw.charge_voltage_limit_v;
    sample->gauge_ok = raw.gauge_ok;
    sample->usb_present = raw.usb_present;
    sample->charging = raw.charging;
    sample->flags = raw.validity_flags;

    // Update adaptive sampling mode based on state
    if (sample->charging) {
        telemetry_set_sampling_mode(SamplingModeCharging);
    } else if (sample->current_a < -0.05f) {
        telemetry_set_sampling_mode(SamplingModeDischarging);
    } else {
        telemetry_set_sampling_mode(SamplingModeIdle);
    }

    return true;
}
