#pragma once
#include <gui/view.h>
#include "../../model/event_model.h"

typedef struct EventsView EventsView;
typedef void (*EventsViewCallback)(void* context);

EventsView* events_view_alloc(void);
void events_view_free(EventsView* instance);
View* events_view_get_view(EventsView* instance);
void events_view_set_callback(EventsView* instance, EventsViewCallback callback, void* context);
void events_view_update(EventsView* instance, EventDataModel* model);
