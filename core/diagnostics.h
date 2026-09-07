#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

// Categorized logging macros with unified tag "BatGuard"
#define BG_LOG_D(format, ...) FURI_LOG_D("BatGuard", format, ##__VA_ARGS__)
#define BG_LOG_I(format, ...) FURI_LOG_I("BatGuard", format, ##__VA_ARGS__)
#define BG_LOG_W(format, ...) FURI_LOG_W("BatGuard", format, ##__VA_ARGS__)
#define BG_LOG_E(format, ...) FURI_LOG_E("BatGuard", format, ##__VA_ARGS__)

typedef struct {
    uint32_t samples_read;
    uint32_t samples_invalid;
    uint32_t sessions_completed;
    uint32_t events_emitted;
    uint32_t journal_writes;
    uint32_t journal_drops;
    uint32_t policy_decisions;
    uint32_t safety_lockouts;
} DiagnosticCounters;

void diagnostics_init(void);
void diagnostics_reset(void);
void diagnostics_free(void);

void diagnostics_inc_samples_read(void);
void diagnostics_inc_samples_invalid(void);
void diagnostics_inc_sessions_completed(void);
void diagnostics_inc_events_emitted(void);
void diagnostics_inc_journal_writes(void);
void diagnostics_inc_journal_drops(void);
void diagnostics_inc_policy_decisions(void);
void diagnostics_inc_safety_lockouts(void);

void diagnostics_get_counters(DiagnosticCounters* out_counters);

#ifdef __cplusplus
}
#endif
