#include "diagnostics.h"
#include <string.h>

static DiagnosticCounters g_counters;
static FuriMutex* g_diag_mutex = NULL;

static inline void saturating_inc(uint32_t* val) {
    if(*val < UINT32_MAX) {
        (*val)++;
    }
}

void diagnostics_init(void) {
    if(!g_diag_mutex) {
        g_diag_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    }
    diagnostics_reset();
}

void diagnostics_reset(void) {
    if(g_diag_mutex) {
        furi_mutex_acquire(g_diag_mutex, FuriWaitForever);
    }
    memset(&g_counters, 0, sizeof(DiagnosticCounters));
    if(g_diag_mutex) {
        furi_mutex_release(g_diag_mutex);
    }
}

void diagnostics_free(void) {
    if(g_diag_mutex) {
        furi_mutex_free(g_diag_mutex);
        g_diag_mutex = NULL;
    }
}

void diagnostics_inc_samples_read(void) {
    if(!g_diag_mutex) diagnostics_init();
    furi_mutex_acquire(g_diag_mutex, FuriWaitForever);
    saturating_inc(&g_counters.samples_read);
    furi_mutex_release(g_diag_mutex);
}

void diagnostics_inc_samples_invalid(void) {
    if(!g_diag_mutex) diagnostics_init();
    furi_mutex_acquire(g_diag_mutex, FuriWaitForever);
    saturating_inc(&g_counters.samples_invalid);
    furi_mutex_release(g_diag_mutex);
}

void diagnostics_inc_sessions_completed(void) {
    if(!g_diag_mutex) diagnostics_init();
    furi_mutex_acquire(g_diag_mutex, FuriWaitForever);
    saturating_inc(&g_counters.sessions_completed);
    furi_mutex_release(g_diag_mutex);
}

void diagnostics_inc_events_emitted(void) {
    if(!g_diag_mutex) diagnostics_init();
    furi_mutex_acquire(g_diag_mutex, FuriWaitForever);
    saturating_inc(&g_counters.events_emitted);
    furi_mutex_release(g_diag_mutex);
}

void diagnostics_inc_journal_writes(void) {
    if(!g_diag_mutex) diagnostics_init();
    furi_mutex_acquire(g_diag_mutex, FuriWaitForever);
    saturating_inc(&g_counters.journal_writes);
    furi_mutex_release(g_diag_mutex);
}

void diagnostics_inc_journal_drops(void) {
    if(!g_diag_mutex) diagnostics_init();
    furi_mutex_acquire(g_diag_mutex, FuriWaitForever);
    saturating_inc(&g_counters.journal_drops);
    furi_mutex_release(g_diag_mutex);
}

void diagnostics_inc_policy_decisions(void) {
    if(!g_diag_mutex) diagnostics_init();
    furi_mutex_acquire(g_diag_mutex, FuriWaitForever);
    saturating_inc(&g_counters.policy_decisions);
    furi_mutex_release(g_diag_mutex);
}

void diagnostics_inc_safety_lockouts(void) {
    if(!g_diag_mutex) diagnostics_init();
    furi_mutex_acquire(g_diag_mutex, FuriWaitForever);
    saturating_inc(&g_counters.safety_lockouts);
    furi_mutex_release(g_diag_mutex);
}

void diagnostics_get_counters(DiagnosticCounters* out_counters) {
    if(!out_counters) return;
    if(!g_diag_mutex) diagnostics_init();
    furi_mutex_acquire(g_diag_mutex, FuriWaitForever);
    *out_counters = g_counters;
    furi_mutex_release(g_diag_mutex);
}
