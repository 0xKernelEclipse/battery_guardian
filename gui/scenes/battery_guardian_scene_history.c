#include "../battery_guardian_app_i.h"
#include "battery_guardian_scene.h"

void history_view_cb(void* context, uint32_t event) {
    BatteryGuardianApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, event);
}

void battery_guardian_scene_history_on_enter(void* context) {
    BatteryGuardianApp* app = context;
    history_view_update(app->history, app->history_model);
    history_view_set_callback(app->history, history_view_cb, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, BatteryGuardianViewHistory);
}

bool battery_guardian_scene_history_on_event(void* context, SceneManagerEvent event) {
    BatteryGuardianApp* app = context;
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == InputKeyLeft || event.event == InputKeyRight) {
            HistoryDataSnapshot snap;
            history_model_get_snapshot(app->history_model, &snap);
            HistoryMetric m = snap.metric;
            if (event.event == InputKeyRight) {
                m = (m + 1) % HistoryMetricCount;
            } else {
                m = (m == 0) ? (HistoryMetricCount - 1) : (m - 1);
            }
            history_model_set_metric(app->history_model, m);
            history_view_update(app->history, app->history_model);
        } else if(event.event == InputKeyOk) {
            scene_manager_search_and_switch_to_previous_scene(app->scene_manager, BatteryGuardianSceneMenu);
        }
        return true;
    } else if(event.type == SceneManagerEventTypeTick) {
        history_view_update(app->history, app->history_model);
        return true;
    }
    return false;
}

void battery_guardian_scene_history_on_exit(void* context) { (void)context; }
