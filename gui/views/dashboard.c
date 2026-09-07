#include "dashboard.h"
#include <gui/elements.h>
#include <furi.h>
#include "../battery_guardian_app_i.h"

struct DashboardView {
    View* view;
    DashboardViewCallback callback;
    void* context;
};

typedef struct {
    BatteryTelemetry telemetry;
    BatterySession session;
    uint32_t total_samples;
    BatteryHealthSnapshot health;
} DashboardViewModel;

static void dashboard_view_draw_callback(Canvas* canvas, void* model) {
    DashboardViewModel* m = model;
    
    canvas_clear(canvas);
    
    // Header
    canvas_set_font(canvas, FontPrimary);
    if(m->total_samples == 0) {
        canvas_draw_str(canvas, 2, 10, "REALITY (WAIT)");
    } else if((m->telemetry.flags & VALID_GAUGE) && !m->telemetry.gauge_ok) {
        canvas_draw_str(canvas, 2, 10, "REALITY [FAULT]");
    } else {
        canvas_draw_str(canvas, 2, 10, "BATTERY REALITY");
    }
    
    // SOC (Top Right)
    canvas_set_font(canvas, FontBigNumbers);
    char str_buf[32];
    if(m->telemetry.flags & VALID_SOC) {
        snprintf(str_buf, sizeof(str_buf), "%d%%", m->telemetry.soc_pct);
    } else {
        snprintf(str_buf, sizeof(str_buf), "--%%");
    }
    canvas_draw_str_aligned(canvas, 126, 12, AlignRight, AlignBottom, str_buf);
    
    // V, I, T Grid
    canvas_set_font(canvas, FontSecondary);
    if(m->telemetry.flags & VALID_VOLTAGE) {
        snprintf(str_buf, sizeof(str_buf), "%d.%02dV", (int)m->telemetry.voltage_v, (int)(m->telemetry.voltage_v * 100.0f) % 100);
    } else {
        snprintf(str_buf, sizeof(str_buf), "--V");
    }
    canvas_draw_str(canvas, 2, 24, str_buf);
    
    if(m->telemetry.flags & VALID_CURRENT) {
        snprintf(str_buf, sizeof(str_buf), "%dmA", (int)(m->telemetry.current_a * 1000.0f));
    } else {
        snprintf(str_buf, sizeof(str_buf), "--mA");
    }
    canvas_draw_str(canvas, 45, 24, str_buf);
    
    if(m->telemetry.flags & VALID_TEMP) {
        snprintf(str_buf, sizeof(str_buf), "%d.%01d C", (int)m->telemetry.temperature_c, (int)(m->telemetry.temperature_c * 10.0f) % 10);
    } else {
        snprintf(str_buf, sizeof(str_buf), "-- C");
    }
    canvas_draw_str_aligned(canvas, 126, 24, AlignRight, AlignBottom, str_buf);
    
    // GAUGE & OBSERVED
    canvas_draw_str(canvas, 2, 34, "GAUGE");
    if(m->telemetry.flags & VALID_HEALTH) {
        snprintf(str_buf, sizeof(str_buf), "%d%%", m->telemetry.gauge_health_pct);
    } else {
        snprintf(str_buf, sizeof(str_buf), "--%%");
    }
    canvas_draw_str_aligned(canvas, 60, 34, AlignRight, AlignBottom, str_buf);
    
    canvas_draw_str(canvas, 2, 43, "OBSERVED");
    if(m->health.estimate_available) {
        snprintf(str_buf, sizeof(str_buf), "%d%%", m->health.observed_health_pct);
    } else {
        snprintf(str_buf, sizeof(str_buf), "--%%");
    }
    canvas_draw_str_aligned(canvas, 60, 43, AlignRight, AlignBottom, str_buf);
    
    canvas_draw_str(canvas, 2, 52, "CAPACITY");
    if(m->health.estimate_available) {
        snprintf(str_buf, sizeof(str_buf), "~%d.%02dAh", (int)(m->health.estimated_capacity_mah / 1000.0f), (int)(m->health.estimated_capacity_mah / 10.0f) % 100);
    } else {
        snprintf(str_buf, sizeof(str_buf), "--Ah");
    }
    canvas_draw_str_aligned(canvas, 60, 52, AlignRight, AlignBottom, str_buf);
    
    // Right column: Trend & Confidence
    canvas_draw_str(canvas, 65, 34, "TREND");
    const char* trend_str = "-";
    if(m->health.trend_available) {
        if(m->health.degradation.trend == DegradationTrendDeclining) trend_str = "DOWN";
        else if(m->health.degradation.trend == DegradationTrendStable) trend_str = "STABLE";
        else if(m->health.degradation.trend == DegradationTrendImproving) trend_str = "UP";
    }
    canvas_draw_str_aligned(canvas, 126, 34, AlignRight, AlignBottom, trend_str);
    
    canvas_draw_str(canvas, 65, 43, "CONF.");
    const char* conf_str = "NONE";
    if(m->health.estimate_available) {
        if(m->health.confidence.overall == ConfidenceLevelHigh) conf_str = "HIGH";
        else if(m->health.confidence.overall == ConfidenceLevelMedium) conf_str = "MED";
        else if(m->health.confidence.overall == ConfidenceLevelLow) conf_str = "LOW";
    }
    canvas_draw_str_aligned(canvas, 126, 43, AlignRight, AlignBottom, conf_str);
    
    const char* state_str = "UNKNOWN";
    if (m->session.is_active) {
        if (m->session.type == SessionTypeCharging) state_str = "CHG";
        else if (m->session.type == SessionTypeDischarging) state_str = "DIS";
        else if (m->session.type == SessionTypeIdle) state_str = "IDLE";
    }
    snprintf(str_buf, sizeof(str_buf), "S %s", state_str);
    canvas_draw_str_aligned(canvas, 126, 52, AlignRight, AlignBottom, str_buf);
    
    // Buttons Hints
    elements_button_left(canvas, "Health");
    elements_button_right(canvas, "Menu");
}

static bool dashboard_view_input_callback(InputEvent* event, void* context) {
    DashboardView* instance = context;
    if(event->type == InputTypeShort) {
        if(instance->callback) {
            if(event->key == InputKeyLeft) {
                instance->callback(instance->context, EventDashboardToHealth);
                return true;
            } else if(event->key == InputKeyRight) {
                instance->callback(instance->context, EventDashboardToMenu);
                return true;
            } else if(event->key == InputKeyOk) {
                instance->callback(instance->context, EventDashboardToMenu);
                return true;
            }
        }
    }
    return false;
}

DashboardView* dashboard_view_alloc(void) {
    DashboardView* instance = malloc(sizeof(DashboardView));
    instance->view = view_alloc();
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(DashboardViewModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, dashboard_view_draw_callback);
    view_set_input_callback(instance->view, dashboard_view_input_callback);
    return instance;
}

void dashboard_view_free(DashboardView* instance) {
    furi_assert(instance);
    view_free(instance->view);
    free(instance);
}

View* dashboard_view_get_view(DashboardView* instance) {
    furi_assert(instance);
    return instance->view;
}

void dashboard_view_set_callback(DashboardView* instance, DashboardViewCallback callback, void* context) {
    furi_assert(instance);
    instance->callback = callback;
    instance->context = context;
}

void dashboard_view_update(DashboardView* instance, BatteryModel* model) {
    furi_assert(instance);
    with_view_model(instance->view, DashboardViewModel* m, {
        BatteryModelSnapshot snap;
        battery_model_get_snapshot(model, &snap);
        memcpy(&m->telemetry, &snap.telemetry, sizeof(BatteryTelemetry));
        memcpy(&m->session, &snap.active_session, sizeof(BatterySession));
        m->total_samples = snap.sample_count;
        memcpy(&m->health, &snap.health, sizeof(BatteryHealthSnapshot));
    }, true);
}
