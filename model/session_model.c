#include "session_model.h"
#include <stdlib.h>
#include <string.h>

struct SessionDataModel {
    BatterySession sessions[MAX_SESSIONS];
    uint32_t head;
    uint32_t count;
    FuriMutex* mutex;
};

SessionDataModel* session_model_alloc(void) {
    SessionDataModel* model = malloc(sizeof(SessionDataModel));
    memset(model, 0, sizeof(SessionDataModel));
    model->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    return model;
}

void session_model_free(SessionDataModel* model) {
    furi_mutex_free(model->mutex);
    free(model);
}

void session_model_add_session(SessionDataModel* model, const BatterySession* session) {
    furi_mutex_acquire(model->mutex, FuriWaitForever);
    
    // Check if updating the current active session
    if (model->count > 0) {
        uint32_t last_idx = (model->head + MAX_SESSIONS - 1) % MAX_SESSIONS;
        if (model->sessions[last_idx].session_id == session->session_id) {
            // Update in place
            model->sessions[last_idx] = *session;
            furi_mutex_release(model->mutex);
            return;
        }
    }
    
    // New session
    model->sessions[model->head] = *session;
    model->head = (model->head + 1) % MAX_SESSIONS;
    if (model->count < MAX_SESSIONS) {
        model->count++;
    }
    furi_mutex_release(model->mutex);
}

void session_model_get_snapshot(SessionDataModel* model, SessionDataSnapshot* snapshot) {
    if(!snapshot) return;
    
    furi_mutex_acquire(model->mutex, FuriWaitForever);
    snapshot->count = 0;
    
    // Copy in reverse order (newest first)
    for(uint32_t i=0; i<model->count; i++) {
        uint32_t idx = (model->head + MAX_SESSIONS - 1 - i) % MAX_SESSIONS;
        snapshot->sessions[i] = model->sessions[idx];
        snapshot->count++;
    }
    
    furi_mutex_release(model->mutex);
}
