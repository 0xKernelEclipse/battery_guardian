#include "trace.h"
#include <string.h>

static uint32_t g_sim_seed = 1;

void sim_prng_seed(uint32_t seed) {
    g_sim_seed = seed;
}

uint32_t sim_prng_next(void) {
    // Simple LCG (Linear Congruential Generator)
    // parameters from Numerical Recipes
    g_sim_seed = g_sim_seed * 1664525 + 1013904223;
    return g_sim_seed;
}

float sim_prng_next_float(void) {
    return (float)sim_prng_next() / (float)0xFFFFFFFF;
}

float sim_prng_range(float min, float max) {
    return min + sim_prng_next_float() * (max - min);
}

void trace_to_telemetry(const SimulatedTelemetry* sim, BatteryTelemetry* out_tel) {
    memset(out_tel, 0, sizeof(BatteryTelemetry));
    out_tel->timestamp_ms = sim->timestamp_ms;
    out_tel->voltage_v = sim->voltage_v;
    out_tel->current_a = sim->current_a;
    out_tel->temperature_c = sim->temperature_c;
    out_tel->soc_pct = sim->soc_pct;
    out_tel->remaining_capacity_mah = sim->remaining_capacity_mah;
    out_tel->full_capacity_mah = sim->full_capacity_mah;
    out_tel->design_capacity_mah = sim->design_capacity_mah;
    out_tel->gauge_health_pct = sim->gauge_health_pct;
    out_tel->gauge_ok = sim->gauge_ok;
    out_tel->charging = sim->charging;
    out_tel->usb_present = sim->usb_present;
    out_tel->flags = sim->flags;
    // Assume standard 4.2V limit for simulations unless otherwise needed
    out_tel->charge_voltage_limit_v = 4.20f; 
}
