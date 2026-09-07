#include "../battery_guardian_app_i.h"
#include "battery_guardian_scene.h"

void diagnostics_view_cb(void* context) {
    BatteryGuardianApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, InputKeyOk);
}

void battery_guardian_scene_diagnostics_on_enter(void* context) {
    BatteryGuardianApp* app = context;
    diagnostics_view_update(app->diagnostics, app->battery_model);
    diagnostics_view_set_callback(app->diagnostics, diagnostics_view_cb, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, BatteryGuardianViewDiagnostics);
}

bool battery_guardian_scene_diagnostics_on_event(void* context, SceneManagerEvent event) {
    BatteryGuardianApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == InputKeyOk) {
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, BatteryGuardianSceneMenu);
        return true;
    } else if(event.type == SceneManagerEventTypeTick) {
        diagnostics_view_update(app->diagnostics, app->battery_model);
        return true;
    }
    return false;
}

void battery_guardian_scene_diagnostics_on_exit(void* context) { (void)context; }
