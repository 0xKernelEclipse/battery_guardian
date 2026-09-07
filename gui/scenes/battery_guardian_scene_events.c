#include "../battery_guardian_app_i.h"
#include "battery_guardian_scene.h"

void events_view_cb(void* context) {
    BatteryGuardianApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, InputKeyOk);
}

void battery_guardian_scene_events_on_enter(void* context) {
    BatteryGuardianApp* app = context;
    events_view_update(app->events, app->event_model);
    events_view_set_callback(app->events, events_view_cb, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, BatteryGuardianViewEvents);
}

bool battery_guardian_scene_events_on_event(void* context, SceneManagerEvent event) {
    BatteryGuardianApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == InputKeyOk) {
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, BatteryGuardianSceneMenu);
        return true;
    } else if(event.type == SceneManagerEventTypeTick) {
        events_view_update(app->events, app->event_model);
        return true;
    }
    return false;
}

void battery_guardian_scene_events_on_exit(void* context) { (void)context; }
