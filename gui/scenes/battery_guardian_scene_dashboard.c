#include "../battery_guardian_app_i.h"
#include "battery_guardian_scene.h"

void battery_guardian_dashboard_custom_event_callback(void* context, uint32_t event) {
    BatteryGuardianApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, event);
}

void battery_guardian_scene_dashboard_on_enter(void* context) {
    BatteryGuardianApp* app = context;
    
    // Wire up callback
    dashboard_view_set_callback(app->dashboard, battery_guardian_dashboard_custom_event_callback, app);
    
    // Explicit initial update
    dashboard_view_update(app->dashboard, app->battery_model);
    
    view_dispatcher_switch_to_view(app->view_dispatcher, BatteryGuardianViewDashboard);
}

bool battery_guardian_scene_dashboard_on_event(void* context, SceneManagerEvent event) {
    BatteryGuardianApp* app = context;
    bool consumed = false;
    
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == EventDashboardToHealth) {
            scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneHealth);
            consumed = true;
        } else if(event.event == EventDashboardToHistory) {
            scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneHistory);
            consumed = true;
        } else if(event.event == EventDashboardToSessions) {
            scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneSessions);
            consumed = true;
        } else if(event.event == EventDashboardToMenu) {
            scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneMenu);
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        dashboard_view_update(app->dashboard, app->battery_model);
        consumed = true;
    }
    
    return consumed;
}

void battery_guardian_scene_dashboard_on_exit(void* context) {
    (void)context;
}
