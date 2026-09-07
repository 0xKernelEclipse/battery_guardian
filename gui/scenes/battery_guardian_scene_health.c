#include "../battery_guardian_app_i.h"
#include "battery_guardian_scene.h"

static void health_view_custom_callback(void* context, uint32_t event) {
    BatteryGuardianApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, event);
}

void battery_guardian_scene_health_on_enter(void* context) {
    BatteryGuardianApp* app = context;
    health_view_set_callback(app->health, health_view_custom_callback, app);
    health_view_update(app->health, app->battery_model);
    view_dispatcher_switch_to_view(app->view_dispatcher, BatteryGuardianViewHealth);
}

bool battery_guardian_scene_health_on_event(void* context, SceneManagerEvent event) {
    BatteryGuardianApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == 0) {
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
        } else if(event.event == 1) {
            scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneExplanation);
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        health_view_update(app->health, app->battery_model);
        consumed = true;
    }

    return consumed;
}

void battery_guardian_scene_health_on_exit(void* context) {
    (void)context;
}
