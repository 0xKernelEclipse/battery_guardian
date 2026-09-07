#pragma once
#include <gui/view.h>
#include "../../model/battery_model.h"

typedef struct DashboardView DashboardView;
typedef void (*DashboardViewCallback)(void* context, uint32_t event);

DashboardView* dashboard_view_alloc(void);
void dashboard_view_free(DashboardView* instance);
View* dashboard_view_get_view(DashboardView* instance);

void dashboard_view_set_callback(DashboardView* instance, DashboardViewCallback callback, void* context);
void dashboard_view_update(DashboardView* instance, BatteryModel* model);
