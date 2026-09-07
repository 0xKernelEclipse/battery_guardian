#include "../battery_guardian_app_i.h"
#include "battery_guardian_scene.h"

static void explanation_view_custom_callback(void* context, uint32_t event) {
    BatteryGuardianApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, event);
}

void battery_guardian_scene_explanation_on_enter(void* context) {
    BatteryGuardianApp* app = context;
    explanation_view_set_callback(app->explanation, explanation_view_custom_callback, app);
    explanation_view_update(app->explanation, app->battery_model);
    view_dispatcher_switch_to_view(app->view_dispatcher, BatteryGuardianViewExplanation);
}

bool battery_guardian_scene_explanation_on_event(void* context, SceneManagerEvent event) {
    BatteryGuardianApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == 0) {
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        explanation_view_update(app->explanation, app->battery_model);
        consumed = true;
    }

    return consumed;
}

void battery_guardian_scene_explanation_on_exit(void* context) {
    (void)context;
}
