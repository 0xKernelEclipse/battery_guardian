#pragma once
#include <furi.h>
#include <stdint.h>
#include "../core/telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HistoryMetricSoc,
    HistoryMetricTemperature,
    HistoryMetricVoltage,
    HistoryMetricCount
} HistoryMetric;

typedef struct {
    uint8_t x; // 0 to 127 for x pixel
    uint8_t y; // scaled y pixel
} HistoryPoint;

typedef struct {
    HistoryPoint points[128];
    uint16_t count;
    HistoryMetric metric;
    
    float min_val;
    float max_val;
} HistoryDataSnapshot;

typedef struct HistoryModel HistoryModel;

HistoryModel* history_model_alloc(void);
void history_model_free(HistoryModel* model);

void history_model_add_sample(HistoryModel* model, const BatteryTelemetry* sample);
void history_model_set_metric(HistoryModel* model, HistoryMetric metric);
void history_model_get_snapshot(HistoryModel* model, HistoryDataSnapshot* snapshot);

#ifdef __cplusplus
}
#endif
