#pragma once
#include <furi.h>
#include <stdint.h>
#include "../core/event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EventDataModel EventDataModel;

#define MAX_EVENTS 10

typedef struct {
    BatteryEvent events[MAX_EVENTS];
    uint32_t count;
} EventDataSnapshot;

EventDataModel* event_model_alloc(void);
void event_model_free(EventDataModel* model);

void event_model_add_event(EventDataModel* model, const BatteryEvent* event);
void event_model_get_snapshot(EventDataModel* model, EventDataSnapshot* snapshot);

#ifdef __cplusplus
}
#endif
