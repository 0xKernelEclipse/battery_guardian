#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "furi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FuriHalPowerICCharger,
    FuriHalPowerICFuelGauge,
} FuriHalPowerIC;

// Mockable state struct
typedef struct {
    bool gauge_ok;
    uint8_t soc_pct;
    uint8_t health_pct;
    bool is_charging;
    bool is_charging_done;
    bool is_otg_enabled;
    float charge_voltage_limit_v;
    uint32_t remaining_capacity_mah;
    uint32_t full_capacity_mah;
    uint32_t design_capacity_mah;
    float voltage_v[2];      // [Charger, FuelGauge]
    float current_a[2];      // [Charger, FuelGauge]
    float temperature_c[2];  // [Charger, FuelGauge]
    float usb_voltage_v;
} MockPowerState;

extern MockPowerState g_mock_power;

static inline void mock_power_reset_defaults(void) {
    memset(&g_mock_power, 0, sizeof(MockPowerState));
    g_mock_power.gauge_ok = true;
    g_mock_power.soc_pct = 75;
    g_mock_power.health_pct = 95;
    g_mock_power.is_charging = false;
    g_mock_power.is_charging_done = false;
    g_mock_power.is_otg_enabled = false;
    g_mock_power.charge_voltage_limit_v = 4.2f;
    g_mock_power.remaining_capacity_mah = 1575;
    g_mock_power.full_capacity_mah = 2050;
    g_mock_power.design_capacity_mah = 2100;
    g_mock_power.voltage_v[FuriHalPowerICFuelGauge] = 3.95f;
    g_mock_power.voltage_v[FuriHalPowerICCharger] = 3.95f;
    g_mock_power.current_a[FuriHalPowerICFuelGauge] = -0.15f;
    g_mock_power.current_a[FuriHalPowerICCharger] = 0.0f;
    g_mock_power.temperature_c[FuriHalPowerICFuelGauge] = 28.5f;
    g_mock_power.temperature_c[FuriHalPowerICCharger] = 28.5f;
    g_mock_power.usb_voltage_v = 0.0f;
}

static inline bool furi_hal_power_gauge_is_ok(void) {
    return g_mock_power.gauge_ok;
}

static inline uint8_t furi_hal_power_get_pct(void) {
    return g_mock_power.soc_pct;
}

static inline uint8_t furi_hal_power_get_bat_health_pct(void) {
    return g_mock_power.health_pct;
}

static inline bool furi_hal_power_is_charging(void) {
    return g_mock_power.is_charging;
}

static inline bool furi_hal_power_is_charging_done(void) {
    return g_mock_power.is_charging_done;
}

static inline bool furi_hal_power_is_otg_enabled(void) {
    return g_mock_power.is_otg_enabled;
}

static inline float furi_hal_power_get_battery_charge_voltage_limit(void) {
    return g_mock_power.charge_voltage_limit_v;
}

static inline uint32_t furi_hal_power_get_battery_remaining_capacity(void) {
    return g_mock_power.remaining_capacity_mah;
}

static inline uint32_t furi_hal_power_get_battery_full_capacity(void) {
    return g_mock_power.full_capacity_mah;
}

static inline uint32_t furi_hal_power_get_battery_design_capacity(void) {
    return g_mock_power.design_capacity_mah;
}

static inline float furi_hal_power_get_battery_voltage(FuriHalPowerIC ic) {
    return g_mock_power.voltage_v[ic];
}

static inline float furi_hal_power_get_battery_current(FuriHalPowerIC ic) {
    return g_mock_power.current_a[ic];
}

static inline float furi_hal_power_get_battery_temperature(FuriHalPowerIC ic) {
    return g_mock_power.temperature_c[ic];
}

static inline float furi_hal_power_get_usb_voltage(void) {
    return g_mock_power.usb_voltage_v;
}

#ifdef __cplusplus
}
#endif
