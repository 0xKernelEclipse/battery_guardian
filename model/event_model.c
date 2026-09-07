#include "event_model.h"
#include <stdlib.h>
#include <string.h>

struct EventDataModel {
    BatteryEvent events[MAX_EVENTS];
    uint32_t head;
    uint32_t count;
    FuriMutex* mutex;
};

EventDataModel* event_model_alloc(void) {
    EventDataModel* model = malloc(sizeof(EventDataModel));
    memset(model, 0, sizeof(EventDataModel));
    model->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    return model;
}

void event_model_free(EventDataModel* model) {
    furi_mutex_free(model->mutex);
    free(model);
}

void event_model_add_event(EventDataModel* model, const BatteryEvent* event) {
    furi_mutex_acquire(model->mutex, FuriWaitForever);
    
    model->events[model->head] = *event;
    model->head = (model->head + 1) % MAX_EVENTS;
    if (model->count < MAX_EVENTS) {
        model->count++;
    }
    
    furi_mutex_release(model->mutex);
}

void event_model_get_snapshot(EventDataModel* model, EventDataSnapshot* snapshot) {
    if(!snapshot) return;
    
    furi_mutex_acquire(model->mutex, FuriWaitForever);
    snapshot->count = 0;
    
    // Copy in reverse order (newest first)
    for(uint32_t i=0; i<model->count; i++) {
        uint32_t idx = (model->head + MAX_EVENTS - 1 - i) % MAX_EVENTS;
        snapshot->events[i] = model->events[idx];
        snapshot->count++;
    }
    
    furi_mutex_release(model->mutex);
}
