#pragma once
#include <furi.h>
#include "../core/telemetry.h"
#include "../core/session.h"
#include "../storage/journal.h"
#include "history_model.h"
#include "session_model.h"
#include "event_model.h"
#include "../phase2/battery_health.h"
#include "../phase2/charge_policy.h"
#include "../core/diagnostics.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BatteryModel BatteryModel;

typedef struct {
    BatteryTelemetry telemetry;
    BatterySession active_session;
    uint32_t sample_count;
    bool storage_ok;
    BatteryHealthSnapshot health;
    ChargePolicyEngine policy_engine;
    DiagnosticCounters diagnostics;
} BatteryModelSnapshot;

BatteryModel* battery_model_alloc(Storage* storage, HistoryModel* h_mod, SessionDataModel* s_mod, EventDataModel* e_mod);
void battery_model_free(BatteryModel* model);

void battery_model_get_snapshot(BatteryModel* model, BatteryModelSnapshot* snapshot);

void battery_model_start_polling(BatteryModel* model);
void battery_model_stop_polling(BatteryModel* model);

#ifdef __cplusplus
}
#endif
