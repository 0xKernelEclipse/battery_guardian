#include "../battery_guardian_app_i.h"
#include "battery_guardian_scene.h"

void sessions_view_cb(void* context) {
    BatteryGuardianApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, InputKeyOk);
}

void battery_guardian_scene_sessions_on_enter(void* context) {
    BatteryGuardianApp* app = context;
    sessions_view_update(app->sessions, app->session_model);
    sessions_view_set_callback(app->sessions, sessions_view_cb, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, BatteryGuardianViewSessions);
}

bool battery_guardian_scene_sessions_on_event(void* context, SceneManagerEvent event) {
    BatteryGuardianApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == InputKeyOk) {
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, BatteryGuardianSceneMenu);
        return true;
    } else if(event.type == SceneManagerEventTypeTick) {
        sessions_view_update(app->sessions, app->session_model);
        return true;
    }
    return false;
}

void battery_guardian_scene_sessions_on_exit(void* context) { (void)context; }
