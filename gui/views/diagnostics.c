#include "diagnostics.h"
#include <gui/elements.h>
#include <furi.h>
#include "../battery_guardian_app_i.h"

struct DiagnosticsView {
    View* view;
    DiagnosticsViewCallback callback;
    void* context;
};

typedef struct {
    BatteryModelSnapshot snapshot;
} DiagnosticsViewModel;

static void diagnostics_view_draw_callback(Canvas* canvas, void* model) {
    DiagnosticsViewModel* m = model;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "DIAGNOSTICS & SAFETY");
    
    canvas_set_font(canvas, FontSecondary);
    
    bool gauge_ok = (m->snapshot.telemetry.flags & VALID_GAUGE) && m->snapshot.telemetry.gauge_ok;
    
    char buf[48];
    snprintf(buf, sizeof(buf), "Gauge: %s  SD: %s", gauge_ok ? "OK" : "FAIL", m->snapshot.storage_ok ? "OK" : "FAIL");
    canvas_draw_str(canvas, 2, 20, buf);
    
    const char* p_name = charge_policy_type_name(m->snapshot.policy_engine.policy.type);
    const char* s_name = charge_policy_state_name(m->snapshot.policy_engine.state);
    snprintf(buf, sizeof(buf), "Pol: %s (%s)", p_name, s_name);
    canvas_draw_str(canvas, 2, 29, buf);

    canvas_draw_str(canvas, 2, 38, "Ctrl: Passive (Safe)");
    
    snprintf(buf, sizeof(buf), "Smpl: %lu  Bad: %lu", 
             (unsigned long)m->snapshot.diagnostics.samples_read, 
             (unsigned long)m->snapshot.diagnostics.samples_invalid);
    canvas_draw_str(canvas, 2, 47, buf);
    
    snprintf(buf, sizeof(buf), "Sess: %lu  Locks: %lu", 
             (unsigned long)m->snapshot.diagnostics.sessions_completed, 
             (unsigned long)m->snapshot.diagnostics.safety_lockouts);
    canvas_draw_str(canvas, 2, 56, buf);
    
    elements_button_center(canvas, "Menu");
}

static bool diagnostics_view_input_callback(InputEvent* event, void* context) {
    DiagnosticsView* instance = context;
    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        if(instance->callback) instance->callback(instance->context);
        return true;
    }
    return false;
}

DiagnosticsView* diagnostics_view_alloc(void) {
    DiagnosticsView* instance = malloc(sizeof(DiagnosticsView));
    instance->view = view_alloc();
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(DiagnosticsViewModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, diagnostics_view_draw_callback);
    view_set_input_callback(instance->view, diagnostics_view_input_callback);
    return instance;
}

void diagnostics_view_free(DiagnosticsView* instance) {
    view_free(instance->view);
    free(instance);
}

View* diagnostics_view_get_view(DiagnosticsView* instance) {
    return instance->view;
}

void diagnostics_view_set_callback(DiagnosticsView* instance, DiagnosticsViewCallback callback, void* context) {
    instance->callback = callback;
    instance->context = context;
}

void diagnostics_view_update(DiagnosticsView* instance, BatteryModel* model) {
    with_view_model(instance->view, DiagnosticsViewModel* m, {
        battery_model_get_snapshot(model, &m->snapshot);
    }, true);
}
