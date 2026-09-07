#include "../battery_guardian_app_i.h"
#include "battery_guardian_scene.h"

enum SubmenuIndex {
    SubmenuIndexBatteryReality,
    SubmenuIndexHistory,
    SubmenuIndexSessions,
    SubmenuIndexDiagnostics,
    SubmenuIndexEvents,
    SubmenuIndexHealth,
};

static void battery_guardian_scene_menu_submenu_callback(void* context, uint32_t index) {
    BatteryGuardianApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void battery_guardian_scene_menu_on_enter(void* context) {
    BatteryGuardianApp* app = context;

    submenu_add_item(app->submenu, "Battery Reality", SubmenuIndexBatteryReality, battery_guardian_scene_menu_submenu_callback, app);
    submenu_add_item(app->submenu, "Battery Health", SubmenuIndexHealth, battery_guardian_scene_menu_submenu_callback, app);
    submenu_add_item(app->submenu, "History", SubmenuIndexHistory, battery_guardian_scene_menu_submenu_callback, app);
    submenu_add_item(app->submenu, "Sessions", SubmenuIndexSessions, battery_guardian_scene_menu_submenu_callback, app);
    submenu_add_item(app->submenu, "Diagnostics", SubmenuIndexDiagnostics, battery_guardian_scene_menu_submenu_callback, app);
    submenu_add_item(app->submenu, "Events", SubmenuIndexEvents, battery_guardian_scene_menu_submenu_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, BatteryGuardianViewMenu);
}

bool battery_guardian_scene_menu_on_event(void* context, SceneManagerEvent event) {
    BatteryGuardianApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubmenuIndexBatteryReality) {
            scene_manager_search_and_switch_to_previous_scene(app->scene_manager, BatteryGuardianSceneDashboard);
            consumed = true;
        } else if(event.event == SubmenuIndexHistory) {
            scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneHistory);
            consumed = true;
        } else if(event.event == SubmenuIndexSessions) {
            scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneSessions);
            consumed = true;
        } else if(event.event == SubmenuIndexDiagnostics) {
            scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneDiagnostics);
            consumed = true;
        } else if(event.event == SubmenuIndexEvents) {
            scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneEvents);
            consumed = true;
        } else if(event.event == SubmenuIndexHealth) {
            scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneHealth);
            consumed = true;
        }
    }

    return consumed;
}

void battery_guardian_scene_menu_on_exit(void* context) {
    BatteryGuardianApp* app = context;
    submenu_reset(app->submenu);
}

