#include "history.h"
#include <gui/elements.h>
#include <furi.h>

struct HistoryView {
    View* view;
    HistoryViewCallback callback;
    void* context;
};

typedef struct {
    HistoryDataSnapshot snapshot;
} HistoryViewModel;

static void history_view_draw_callback(Canvas* canvas, void* model) {
    HistoryViewModel* m = model;
    
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    char title[32];
    if (m->snapshot.metric == HistoryMetricSoc) snprintf(title, sizeof(title), "HISTORY - SOC");
    else if (m->snapshot.metric == HistoryMetricTemperature) snprintf(title, sizeof(title), "HISTORY - TEMP");
    else if (m->snapshot.metric == HistoryMetricVoltage) snprintf(title, sizeof(title), "HISTORY - VOLT");
    canvas_draw_str(canvas, 2, 10, title);
    
    // Y-axis labels
    canvas_set_font(canvas, FontSecondary);
    char buf[16];
    if (m->snapshot.metric == HistoryMetricSoc) {
        snprintf(buf, sizeof(buf), "%d", (int)m->snapshot.max_val);
        canvas_draw_str(canvas, 2, 20, buf);
        snprintf(buf, sizeof(buf), "%d", (int)m->snapshot.min_val);
        canvas_draw_str(canvas, 2, 50, buf);
    } else {
        snprintf(buf, sizeof(buf), "%d.%d", (int)m->snapshot.max_val, (int)(m->snapshot.max_val * 10.0f) % 10);
        canvas_draw_str(canvas, 2, 20, buf);
        snprintf(buf, sizeof(buf), "%d.%d", (int)m->snapshot.min_val, (int)(m->snapshot.min_val * 10.0f) % 10);
        canvas_draw_str(canvas, 2, 50, buf);
    }
    
    // Draw grid/frame
    canvas_draw_line(canvas, 20, 10, 20, 50);
    canvas_draw_line(canvas, 20, 50, 127, 50);
    
    // Draw points
    for(uint32_t i = 1; i < m->snapshot.count; i++) {
        uint8_t x0 = 20 + (m->snapshot.points[i-1].x * 107) / 127;
        uint8_t y0 = m->snapshot.points[i-1].y;
        uint8_t x1 = 20 + (m->snapshot.points[i].x * 107) / 127;
        uint8_t y1 = m->snapshot.points[i].y;
        
        canvas_draw_line(canvas, x0, y0, x1, y1);
    }
    
    elements_button_center(canvas, "Menu");
    elements_button_left(canvas, "<");
    elements_button_right(canvas, ">");
}

static bool history_view_input_callback(InputEvent* event, void* context) {
    HistoryView* instance = context;
    if(event->type == InputTypeShort) {
        if(instance->callback) {
            instance->callback(instance->context, event->key);
            return true;
        }
    }
    return false;
}

HistoryView* history_view_alloc(void) {
    HistoryView* instance = malloc(sizeof(HistoryView));
    instance->view = view_alloc();
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(HistoryViewModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, history_view_draw_callback);
    view_set_input_callback(instance->view, history_view_input_callback);
    return instance;
}

void history_view_free(HistoryView* instance) {
    view_free(instance->view);
    free(instance);
}

View* history_view_get_view(HistoryView* instance) {
    return instance->view;
}

void history_view_set_callback(HistoryView* instance, HistoryViewCallback callback, void* context) {
    instance->callback = callback;
    instance->context = context;
}

void history_view_update(HistoryView* instance, HistoryModel* model) {
    with_view_model(instance->view, HistoryViewModel* m, {
        history_model_get_snapshot(model, &m->snapshot);
    }, true);
}
