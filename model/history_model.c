#include "history_model.h"
#include "../storage/journal.h"
#include "../core/telemetry.h"
#include <stdlib.h>
#include <string.h>

#define MAX_HISTORY_POINTS 128

struct HistoryModel {
    BatteryTelemetry raw_samples[MAX_HISTORY_POINTS];
    uint32_t head;
    uint32_t count;
    
    HistoryMetric current_metric;
    FuriMutex* mutex;
};

HistoryModel* history_model_alloc(void) {
    HistoryModel* model = malloc(sizeof(HistoryModel));
    memset(model, 0, sizeof(HistoryModel));
    model->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    model->current_metric = HistoryMetricSoc;
    return model;
}

void history_model_free(HistoryModel* model) {
    furi_mutex_free(model->mutex);
    free(model);
}

void history_model_add_sample(HistoryModel* model, const BatteryTelemetry* sample) {
    furi_mutex_acquire(model->mutex, FuriWaitForever);
    model->raw_samples[model->head] = *sample;
    model->head = (model->head + 1) % MAX_HISTORY_POINTS;
    if (model->count < MAX_HISTORY_POINTS) {
        model->count++;
    }
    furi_mutex_release(model->mutex);
}

void history_model_set_metric(HistoryModel* model, HistoryMetric metric) {
    furi_mutex_acquire(model->mutex, FuriWaitForever);
    model->current_metric = metric;
    furi_mutex_release(model->mutex);
}

void history_model_get_snapshot(HistoryModel* model, HistoryDataSnapshot* snapshot) {
    if(!snapshot) return;
    
    furi_mutex_acquire(model->mutex, FuriWaitForever);
    
    snapshot->metric = model->current_metric;
    snapshot->count = 0;
    
    if (model->count > 0) {
        float min_v = 9999.0f, max_v = -9999.0f;
        
        // Find bounds
        for(uint32_t i=0; i<model->count; i++) {
            uint32_t idx = (model->head + MAX_HISTORY_POINTS - model->count + i) % MAX_HISTORY_POINTS;
            float val = 0.0f;
            if(snapshot->metric == HistoryMetricSoc) val = model->raw_samples[idx].soc_pct;
            else if(snapshot->metric == HistoryMetricTemperature) val = model->raw_samples[idx].temperature_c;
            else if(snapshot->metric == HistoryMetricVoltage) val = model->raw_samples[idx].voltage_v;
            
            if(val < min_v) min_v = val;
            if(val > max_v) max_v = val;
        }
        
        if(snapshot->metric == HistoryMetricSoc) { min_v = 0; max_v = 100; }
        else if(min_v == max_v) { min_v -= 1.0f; max_v += 1.0f; }
        
        snapshot->min_val = min_v;
        snapshot->max_val = max_v;
        
        float range = max_v - min_v;
        if(range < 0.01f) range = 0.01f;
        
        // Generate decimated points
        for(uint32_t i=0; i<model->count; i++) {
            uint32_t idx = (model->head + MAX_HISTORY_POINTS - model->count + i) % MAX_HISTORY_POINTS;
            float val = 0.0f;
            if(snapshot->metric == HistoryMetricSoc) val = model->raw_samples[idx].soc_pct;
            else if(snapshot->metric == HistoryMetricTemperature) val = model->raw_samples[idx].temperature_c;
            else if(snapshot->metric == HistoryMetricVoltage) val = model->raw_samples[idx].voltage_v;
            
            snapshot->points[i].x = (i * 127) / (model->count > 1 ? model->count - 1 : 1);
            snapshot->points[i].y = 50 - (uint8_t)(((val - min_v) / range) * 40.0f);
        }
        snapshot->count = model->count;
    }
    
    furi_mutex_release(model->mutex);
}
