#include "events.h"
#include <gui/elements.h>
#include <furi.h>

struct EventsView {
    View* view;
    EventsViewCallback callback;
    void* context;
};

typedef struct {
    EventDataSnapshot snapshot;
} EventsViewModel;

static void events_view_draw_callback(Canvas* canvas, void* model) {
    EventsViewModel* m = model;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "EVENTS");
    
    canvas_set_font(canvas, FontSecondary);
    
    if (m->snapshot.count == 0) {
        canvas_draw_str(canvas, 2, 22, "No events recorded");
    } else {
        uint8_t y = 22;
        for(uint32_t i=0; i < m->snapshot.count && i < 4; i++) {
            BatteryEvent* e = &m->snapshot.events[i];
            char buf[64];
            const char* name = "UNKNOWN";
            if (e->type == EventTypeChargeStarted) name = "CHARGE START";
            else if (e->type == EventTypeChargeEnded) name = "CHARGE END";
            else if (e->type == EventTypeDischargeStarted) name = "DISCHARGE";
            else if (e->type == EventTypeTemperatureSpike) name = "TEMP WARN";
            
            // Format time roughly (mock time for now as we just have ms tick in timestamps)
            uint32_t s = e->timestamp_ms / 1000;
            snprintf(buf, sizeof(buf), "%02lu:%02lu %s", (s/3600)%24, (s/60)%60, name);
            canvas_draw_str(canvas, 2, y, buf);
            y += 10;
        }
    }
    
    elements_button_center(canvas, "Menu");
}

static bool events_view_input_callback(InputEvent* event, void* context) {
    EventsView* instance = context;
    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        if(instance->callback) instance->callback(instance->context);
        return true;
    }
    return false;
}

EventsView* events_view_alloc(void) {
    EventsView* instance = malloc(sizeof(EventsView));
    instance->view = view_alloc();
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(EventsViewModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, events_view_draw_callback);
    view_set_input_callback(instance->view, events_view_input_callback);
    return instance;
}

void events_view_free(EventsView* instance) {
    view_free(instance->view);
    free(instance);
}

View* events_view_get_view(EventsView* instance) {
    return instance->view;
}

void events_view_set_callback(EventsView* instance, EventsViewCallback callback, void* context) {
    instance->callback = callback;
    instance->context = context;
}

void events_view_update(EventsView* instance, EventDataModel* model) {
    with_view_model(instance->view, EventsViewModel* m, {
        event_model_get_snapshot(model, &m->snapshot);
    }, true);
}
