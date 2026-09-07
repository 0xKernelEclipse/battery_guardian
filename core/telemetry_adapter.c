#include "telemetry_adapter.h"
#include "telemetry.h"
#include <furi.h>
#include <furi_hal_power.h>
#include <string.h>

static bool default_flipper_read(TelemetryRawSample* out_sample, void* context);
static uint32_t default_flipper_get_capabilities(void* context);
static TelemetryState default_flipper_get_state(void* context);

static const TelemetryAdapterInterface default_interface = {
    .read = default_flipper_read,
    .get_capabilities = default_flipper_get_capabilities,
    .get_state = default_flipper_get_state,
};

static const TelemetryAdapterInterface* current_backend = &default_interface;
static void* current_context = NULL;

static uint32_t last_raw_tick = 0;
static uint64_t rollover_offset_ms = 0;
static bool tick_initialized = false;
static uint64_t last_timestamp_ms = 0;

static uint64_t adapter_get_monotonic_ms(void) {
    uint32_t current_tick = furi_get_tick();
    if (!tick_initialized) {
        last_raw_tick = current_tick;
        tick_initialized = true;
    } else if (current_tick < last_raw_tick) {
        // 32-bit tick counter wrapped around (~49.7 days of continuous runtime)
        rollover_offset_ms += ((uint64_t)1 << 32);
    }
    last_raw_tick = current_tick;
    uint64_t now_ms = rollover_offset_ms + (uint64_t)current_tick;
    if (now_ms < last_timestamp_ms) {
        now_ms = last_timestamp_ms;
    }
    last_timestamp_ms = now_ms;
    return now_ms;
}

// Default Flipper Platform HAL driver
static bool default_flipper_read(TelemetryRawSample* out_sample, void* context) {
    (void)context;
    if (!out_sample) return false;
    memset(out_sample, 0, sizeof(TelemetryRawSample));

    out_sample->timestamp_ms = adapter_get_monotonic_ms();
    out_sample->capabilities = TELEMETRY_CAP_FLIPPER_F7;

    // Check gauge communication: query is always performed
    out_sample->gauge_ok = furi_hal_power_gauge_is_ok();
    out_sample->validity_flags |= VALID_GAUGE;

    // 1. Pack Voltage (Volts)
    out_sample->voltage_v = furi_hal_power_get_battery_voltage(FuriHalPowerICFuelGauge);
    if (out_sample->voltage_v > 0.0f && out_sample->voltage_v < 5.0f) {
        out_sample->validity_flags |= VALID_VOLTAGE;
    }

    // 2. Pack Current (Amperes: positive = charging, negative = discharging)
    out_sample->current_a = furi_hal_power_get_battery_current(FuriHalPowerICFuelGauge);
    if (out_sample->current_a > -5.0f && out_sample->current_a < 5.0f) {
        out_sample->validity_flags |= VALID_CURRENT;
    }

    // 3. Temperature (Celsius)
    out_sample->temperature_c = furi_hal_power_get_battery_temperature(FuriHalPowerICFuelGauge);
    if (out_sample->temperature_c > -20.0f && out_sample->temperature_c < 70.0f) {
        out_sample->validity_flags |= VALID_TEMP;
    }

    // 4. State of Charge (%)
    out_sample->soc_pct = furi_hal_power_get_pct();
    if (out_sample->soc_pct <= 100) {
        out_sample->validity_flags |= VALID_SOC;
    }

    // 5. Capacities (mAh)
    out_sample->remaining_capacity_mah = furi_hal_power_get_battery_remaining_capacity();
    if (out_sample->remaining_capacity_mah > 0) {
        out_sample->validity_flags |= VALID_REMAINING_CAPACITY;
    }

    out_sample->full_capacity_mah = furi_hal_power_get_battery_full_capacity();
    if (out_sample->full_capacity_mah > 0) {
        out_sample->validity_flags |= VALID_FULL_CAPACITY;
    }

    out_sample->design_capacity_mah = furi_hal_power_get_battery_design_capacity();
    if (out_sample->design_capacity_mah > 0) {
        out_sample->validity_flags |= VALID_DESIGN_CAPACITY;
    }

    // 6. Gauge health estimate (%)
    out_sample->gauge_health_pct = furi_hal_power_get_bat_health_pct();
    if (out_sample->gauge_health_pct <= 100) {
        out_sample->validity_flags |= VALID_HEALTH;
    }

    // 7. Power/Charging status
    out_sample->charge_voltage_limit_v = furi_hal_power_get_battery_charge_voltage_limit();
    out_sample->charging = furi_hal_power_is_charging();
    out_sample->charging_done = furi_hal_power_is_charging_done();
    out_sample->usb_voltage_v = furi_hal_power_get_usb_voltage();
    out_sample->usb_present = out_sample->charging || (out_sample->usb_voltage_v > 4.0f) || furi_hal_power_is_otg_enabled();

    // Determine operational state
    bool primary_valid = (out_sample->validity_flags & VALID_VOLTAGE) &&
                         (out_sample->validity_flags & VALID_TEMP) &&
                         (out_sample->validity_flags & VALID_SOC);

    if (!out_sample->gauge_ok) {
        out_sample->state = TelemetryStateFault;
    } else if (!primary_valid) {
        out_sample->state = TelemetryStatePartial;
    } else {
        out_sample->state = TelemetryStateNormal;
    }

    return true;
}

static uint32_t default_flipper_get_capabilities(void* context) {
    (void)context;
    return TELEMETRY_CAP_FLIPPER_F7;
}

static TelemetryState default_flipper_get_state(void* context) {
    (void)context;
    TelemetryRawSample sample;
    if (default_flipper_read(&sample, NULL)) {
        return sample.state;
    }
    return TelemetryStateUnavailable;
}

void telemetry_adapter_init(void) {
    current_backend = &default_interface;
    current_context = NULL;
    tick_initialized = false;
    last_raw_tick = 0;
    rollover_offset_ms = 0;
    last_timestamp_ms = 0;
}

void telemetry_adapter_set_backend(const TelemetryAdapterInterface* iface, void* context) {
    if (iface) {
        current_backend = iface;
        current_context = context;
    } else {
        current_backend = &default_interface;
        current_context = NULL;
    }
}

bool telemetry_adapter_read_sample(TelemetryRawSample* out_sample) {
    if (!current_backend || !current_backend->read) {
        return false;
    }
    return current_backend->read(out_sample, current_context);
}

uint32_t telemetry_adapter_get_capabilities(void) {
    if (!current_backend || !current_backend->get_capabilities) {
        return TELEMETRY_CAP_NONE;
    }
    return current_backend->get_capabilities(current_context);
}

TelemetryState telemetry_adapter_get_state(void) {
    if (!current_backend || !current_backend->get_state) {
        return TelemetryStateUnavailable;
    }
    return current_backend->get_state(current_context);
}
