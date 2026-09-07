#pragma once
#include <gui/view.h>
#include "../../model/battery_model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*HealthViewCallback)(void* context, uint32_t event);

typedef struct HealthView HealthView;

HealthView* health_view_alloc(void);
void health_view_free(HealthView* instance);
View* health_view_get_view(HealthView* instance);
void health_view_set_callback(HealthView* instance, HealthViewCallback callback, void* context);
void health_view_update(HealthView* instance, BatteryModel* model);

#ifdef __cplusplus
}
#endif
