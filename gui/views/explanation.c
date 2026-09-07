#include "explanation.h"
#include <gui/elements.h>
#include <furi.h>
#include <stdio.h>
#include <string.h>
#include "../battery_guardian_app_i.h"

struct ExplanationView {
    View* view;
    ExplanationViewCallback callback;
    void* context;
};

typedef struct {
    BatteryHealthSnapshot health;
} ExplanationViewModel;

static void explanation_view_draw_callback(Canvas* canvas, void* _model) {
    ExplanationViewModel* m = _model;
    char buf[48];

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "WHY?");

    canvas_set_font(canvas, FontSecondary);

    if(!m->health.estimate_available) {
        canvas_draw_str(canvas, 2, 22, "No evidence yet.");
        canvas_draw_str(canvas, 2, 32, "Need completed");
        canvas_draw_str(canvas, 2, 42, "discharge sessions.");
        elements_button_left(canvas, "Back");
        return;
    }

    /* Accepted & rejected sessions */
    snprintf(buf, sizeof(buf), "%lu qualifying sessions",
             (unsigned long)m->health.accepted_sessions);
    canvas_draw_str(canvas, 2, 22, buf);

    snprintf(buf, sizeof(buf), "%lu rejected outliers",
             (unsigned long)m->health.rejected_sessions);
    canvas_draw_str(canvas, 2, 31, buf);

    /* Confidence explanation text from engine */
    /* Split the explanation on newlines manually (no stdlib strtok on Flipper) */
    const char* explanation = m->health.confidence.explanation;
    int y = 40;
    const char* p = explanation;
    while(*p && y <= 58) {
        const char* nl = p;
        while(*nl && *nl != '\n') nl++;
        int len = (int)(nl - p);
        if(len > 0 && len < 40) {
            char line[40];
            memcpy(line, p, (size_t)len);
            line[len] = '\0';
            canvas_draw_str(canvas, 2, y, line);
            y += 9;
        }
        p = (*nl == '\n') ? nl + 1 : nl;
    }

    elements_button_left(canvas, "Back");
}

static bool explanation_view_input_callback(InputEvent* event, void* context) {
    ExplanationView* instance = context;
    if(event->type == InputTypeShort && event->key == InputKeyLeft && instance->callback) {
        instance->callback(instance->context, 0);
        return true;
    }
    return false;
}

ExplanationView* explanation_view_alloc(void) {
    ExplanationView* instance = malloc(sizeof(ExplanationView));
    instance->view = view_alloc();
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(ExplanationViewModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, explanation_view_draw_callback);
    view_set_input_callback(instance->view, explanation_view_input_callback);
    return instance;
}

void explanation_view_free(ExplanationView* instance) {
    view_free(instance->view);
    free(instance);
}

View* explanation_view_get_view(ExplanationView* instance) {
    return instance->view;
}

void explanation_view_set_callback(ExplanationView* instance, ExplanationViewCallback callback, void* context) {
    instance->callback = callback;
    instance->context  = context;
}

void explanation_view_update(ExplanationView* instance, BatteryModel* model) {
    with_view_model(instance->view, ExplanationViewModel* m, {
        BatteryModelSnapshot snap;
        battery_model_get_snapshot(model, &snap);
        memcpy(&m->health, &snap.health, sizeof(BatteryHealthSnapshot));
    }, true);
}
