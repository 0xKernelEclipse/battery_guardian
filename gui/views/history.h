#pragma once
#include <gui/view.h>
#include "../../model/history_model.h"

typedef struct HistoryView HistoryView;
typedef void (*HistoryViewCallback)(void* context, uint32_t event);

HistoryView* history_view_alloc(void);
void history_view_free(HistoryView* instance);
View* history_view_get_view(HistoryView* instance);

void history_view_set_callback(HistoryView* instance, HistoryViewCallback callback, void* context);
void history_view_update(HistoryView* instance, HistoryModel* model);
