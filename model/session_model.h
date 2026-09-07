#pragma once
#include <furi.h>
#include <stdint.h>
#include "../core/session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SessionDataModel SessionDataModel;

#define MAX_SESSIONS 10

typedef struct {
    BatterySession sessions[MAX_SESSIONS];
    uint32_t count;
} SessionDataSnapshot;

SessionDataModel* session_model_alloc(void);
void session_model_free(SessionDataModel* model);

void session_model_add_session(SessionDataModel* model, const BatterySession* session);
void session_model_get_snapshot(SessionDataModel* model, SessionDataSnapshot* snapshot);

#ifdef __cplusplus
}
#endif
