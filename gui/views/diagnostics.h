#pragma once
#include <gui/view.h>
#include "../../model/battery_model.h"

typedef struct DiagnosticsView DiagnosticsView;
typedef void (*DiagnosticsViewCallback)(void* context);

DiagnosticsView* diagnostics_view_alloc(void);
void diagnostics_view_free(DiagnosticsView* instance);
View* diagnostics_view_get_view(DiagnosticsView* instance);
void diagnostics_view_set_callback(DiagnosticsView* instance, DiagnosticsViewCallback callback, void* context);
void diagnostics_view_update(DiagnosticsView* instance, BatteryModel* model);
