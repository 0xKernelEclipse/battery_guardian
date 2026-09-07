#include "sessions.h"
#include <gui/elements.h>
#include <furi.h>

struct SessionsView {
    View* view;
    SessionsViewCallback callback;
    void* context;
};

typedef struct {
    SessionDataSnapshot snapshot;
} SessionsViewModel;

static void sessions_view_draw_callback(Canvas* canvas, void* model) {
    SessionsViewModel* m = model;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "SESSIONS");
    
    canvas_set_font(canvas, FontSecondary);
    
    if (m->snapshot.count == 0) {
        canvas_draw_str(canvas, 2, 22, "No sessions recorded");
    } else {
        uint8_t y = 22;
        for(uint32_t i=0; i < m->snapshot.count && i < 4; i++) {
            BatterySession* s = &m->snapshot.sessions[i];
            char buf[64];
            uint32_t duration_m = (s->end_timestamp - s->start_timestamp) / 60000;
            const char* typ_str = (s->type == SessionTypeCharging) ? "CHG" : "DIS";
            snprintf(buf, sizeof(buf), "#%lu %s  %d->%d%%  %lum", 
                     s->session_id, typ_str, s->start_soc, s->end_soc, duration_m);
            canvas_draw_str(canvas, 2, y, buf);
            y += 10;
        }
    }
    
    elements_button_center(canvas, "Menu");
}

static bool sessions_view_input_callback(InputEvent* event, void* context) {
    SessionsView* instance = context;
    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        if(instance->callback) instance->callback(instance->context);
        return true;
    }
    return false;
}

SessionsView* sessions_view_alloc(void) {
    SessionsView* instance = malloc(sizeof(SessionsView));
    instance->view = view_alloc();
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(SessionsViewModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, sessions_view_draw_callback);
    view_set_input_callback(instance->view, sessions_view_input_callback);
    return instance;
}

void sessions_view_free(SessionsView* instance) {
    view_free(instance->view);
    free(instance);
}

View* sessions_view_get_view(SessionsView* instance) {
    return instance->view;
}

void sessions_view_set_callback(SessionsView* instance, SessionsViewCallback callback, void* context) {
    instance->callback = callback;
    instance->context = context;
}

void sessions_view_update(SessionsView* instance, SessionDataModel* model) {
    with_view_model(instance->view, SessionsViewModel* m, {
        session_model_get_snapshot(model, &m->snapshot);
    }, true);
}
