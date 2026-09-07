#pragma once
#include <gui/view.h>
#include "../../model/session_model.h"

typedef struct SessionsView SessionsView;
typedef void (*SessionsViewCallback)(void* context);

SessionsView* sessions_view_alloc(void);
void sessions_view_free(SessionsView* instance);
View* sessions_view_get_view(SessionsView* instance);

void sessions_view_set_callback(SessionsView* instance, SessionsViewCallback callback, void* context);
void sessions_view_update(SessionsView* instance, SessionDataModel* model);
