#include "health.h"
#include <gui/elements.h>
#include <furi.h>
#include "../battery_guardian_app_i.h"

struct HealthView {
    View* view;
    HealthViewCallback callback;
    void* context;
};

typedef struct {
    BatteryHealthSnapshot health;
} HealthViewModel;

static const char* trend_label(DegradationTrendType t) {
    switch(t) {
    case DegradationTrendDeclining:   return "DECLINING";
    case DegradationTrendStable:      return "STABLE";
    case DegradationTrendImproving:   return "IMPROVING(?)";
    default:                          return "UNKNOWN";
    }
}

static const char* conf_label(ConfidenceLevel l) {
    switch(l) {
    case ConfidenceLevelHigh:         return "HIGH";
    case ConfidenceLevelMedium:       return "MEDIUM";
    case ConfidenceLevelLow:          return "LOW";
    default:                          return "INSUFFICIENT";
    }
}

static void health_view_draw_callback(Canvas* canvas, void* _model) {
    HealthViewModel* m = _model;
    char buf[48];

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "BATTERY HEALTH");

    canvas_set_font(canvas, FontSecondary);

    if(!m->health.estimate_available) {
        canvas_draw_str(canvas, 2, 28, "Collecting data...");
        canvas_draw_str(canvas, 2, 40, "Need qualifying");
        canvas_draw_str(canvas, 2, 50, "discharge sessions.");
        elements_button_left(canvas, "Back");
        return;
    }

    /* Row 1: Observed capacity */
    canvas_draw_str(canvas, 2, 22, "Observed:");
    snprintf(buf, sizeof(buf), "~%lumAh",
             (unsigned long)m->health.estimated_capacity_mah);
    canvas_draw_str_aligned(canvas, 126, 22, AlignRight, AlignBottom, buf);

    /* Row 2: Reference */
    canvas_draw_str(canvas, 2, 31, "Reference:");
    snprintf(buf, sizeof(buf), "%lumAh",
             (unsigned long)m->health.reference_capacity_mah);
    canvas_draw_str_aligned(canvas, 126, 31, AlignRight, AlignBottom, buf);

    /* Row 3: Health */
    canvas_draw_str(canvas, 2, 40, "Health:");
    snprintf(buf, sizeof(buf), "%d%%", m->health.observed_health_pct);
    canvas_draw_str_aligned(canvas, 126, 40, AlignRight, AlignBottom, buf);

    /* Row 4: Trend */
    canvas_draw_str(canvas, 2, 49, "Trend:");
    canvas_draw_str_aligned(canvas, 126, 49, AlignRight, AlignBottom,
        m->health.trend_available ? trend_label(m->health.degradation.trend) : "---");

    /* Row 5: Confidence */
    canvas_draw_str(canvas, 2, 58, "Confidence:");
    canvas_draw_str_aligned(canvas, 126, 58, AlignRight, AlignBottom,
        conf_label(m->health.confidence.overall));

    elements_button_left(canvas, "Back");
    elements_button_right(canvas, "Why?");
}

static bool health_view_input_callback(InputEvent* event, void* context) {
    HealthView* instance = context;
    if(event->type == InputTypeShort && instance->callback) {
        if(event->key == InputKeyLeft) {
            instance->callback(instance->context, 0); /* back */
            return true;
        }
        if(event->key == InputKeyRight) {
            instance->callback(instance->context, 1); /* explanation */
            return true;
        }
    }
    return false;
}

HealthView* health_view_alloc(void) {
    HealthView* instance = malloc(sizeof(HealthView));
    instance->view = view_alloc();
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(HealthViewModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, health_view_draw_callback);
    view_set_input_callback(instance->view, health_view_input_callback);
    return instance;
}

void health_view_free(HealthView* instance) {
    view_free(instance->view);
    free(instance);
}

View* health_view_get_view(HealthView* instance) {
    return instance->view;
}

void health_view_set_callback(HealthView* instance, HealthViewCallback callback, void* context) {
    instance->callback = callback;
    instance->context  = context;
}

void health_view_update(HealthView* instance, BatteryModel* model) {
    with_view_model(instance->view, HealthViewModel* m, {
        BatteryModelSnapshot snap;
        battery_model_get_snapshot(model, &snap);
        memcpy(&m->health, &snap.health, sizeof(BatteryHealthSnapshot));
    }, true);
}
