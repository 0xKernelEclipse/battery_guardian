#pragma once
#include <gui/view.h>
#include "../../model/battery_model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ExplanationViewCallback)(void* context, uint32_t event);

typedef struct ExplanationView ExplanationView;

ExplanationView* explanation_view_alloc(void);
void explanation_view_free(ExplanationView* instance);
View* explanation_view_get_view(ExplanationView* instance);
void explanation_view_set_callback(ExplanationView* instance, ExplanationViewCallback callback, void* context);
void explanation_view_update(ExplanationView* instance, BatteryModel* model);

#ifdef __cplusplus
}
#endif
