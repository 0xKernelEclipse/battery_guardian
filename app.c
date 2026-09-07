#include "gui/battery_guardian_app_i.h"
#include "gui/scenes/battery_guardian_scene.h"
#include "battery_guardian.h"

static bool app_custom_event_callback(void* context, uint32_t event) {
    BatteryGuardianApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool app_back_event_callback(void* context) {
    BatteryGuardianApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void battery_guardian_tick_event_callback(void* context) {
    BatteryGuardianApp* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

static BatteryGuardianApp* battery_guardian_app_alloc(void) {
    BatteryGuardianApp* app = malloc(sizeof(BatteryGuardianApp));
    
    // Core Models
    app->history_model = history_model_alloc();
    app->session_model = session_model_alloc();
    app->event_model = event_model_alloc();
    
    app->storage = furi_record_open(RECORD_STORAGE);
    app->battery_model = battery_model_alloc(app->storage, app->history_model, app->session_model, app->event_model);
    
    // GUI Subsystem
    app->gui = furi_record_open(RECORD_GUI);
    
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&battery_guardian_scene_handlers, app);
    
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, app_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, app_back_event_callback);
    view_dispatcher_set_tick_event_callback(app->view_dispatcher, battery_guardian_tick_event_callback, 500); // 2Hz
    
    // Allocate views
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BatteryGuardianViewMenu, submenu_get_view(app->submenu));
    
    app->dashboard = dashboard_view_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BatteryGuardianViewDashboard, dashboard_view_get_view(app->dashboard));
    
    app->history = history_view_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BatteryGuardianViewHistory, history_view_get_view(app->history));
    
    app->sessions = sessions_view_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BatteryGuardianViewSessions, sessions_view_get_view(app->sessions));
    
    app->diagnostics = diagnostics_view_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BatteryGuardianViewDiagnostics, diagnostics_view_get_view(app->diagnostics));
    
    app->events = events_view_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BatteryGuardianViewEvents, events_view_get_view(app->events));

    app->health = health_view_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BatteryGuardianViewHealth, health_view_get_view(app->health));

    app->explanation = explanation_view_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BatteryGuardianViewExplanation, explanation_view_get_view(app->explanation));

    // Attach dispatcher to GUI
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    return app;
}

static void battery_guardian_app_free(BatteryGuardianApp* app) {
    // Stop models first
    battery_model_stop_polling(app->battery_model);
    
    // Free views
    view_dispatcher_remove_view(app->view_dispatcher, BatteryGuardianViewMenu);
    submenu_free(app->submenu);
    
    view_dispatcher_remove_view(app->view_dispatcher, BatteryGuardianViewDashboard);
    dashboard_view_free(app->dashboard);
    
    view_dispatcher_remove_view(app->view_dispatcher, BatteryGuardianViewHistory);
    history_view_free(app->history);
    
    view_dispatcher_remove_view(app->view_dispatcher, BatteryGuardianViewSessions);
    sessions_view_free(app->sessions);
    
    view_dispatcher_remove_view(app->view_dispatcher, BatteryGuardianViewDiagnostics);
    diagnostics_view_free(app->diagnostics);
    
    view_dispatcher_remove_view(app->view_dispatcher, BatteryGuardianViewEvents);
    events_view_free(app->events);

    view_dispatcher_remove_view(app->view_dispatcher, BatteryGuardianViewHealth);
    health_view_free(app->health);

    view_dispatcher_remove_view(app->view_dispatcher, BatteryGuardianViewExplanation);
    explanation_view_free(app->explanation);

    // Free dispatcher & scene manager
    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);
    
    // Free models
    battery_model_free(app->battery_model);
    history_model_free(app->history_model);
    session_model_free(app->session_model);
    event_model_free(app->event_model);
    
    if (app->storage) {
        furi_record_close(RECORD_STORAGE);
        app->storage = NULL;
    }
    
    furi_record_close(RECORD_GUI);
    
    free(app);
}

// Entry point for the Battery Guardian FAP
int32_t battery_guardian_app(void* p) {
    UNUSED(p);
    
    BatteryGuardianApp* app = battery_guardian_app_alloc();
    
    // Start background polling
    battery_model_start_polling(app->battery_model);
    
    // Start the first scene
    scene_manager_next_scene(app->scene_manager, BatteryGuardianSceneDashboard);
    
    // Run the view dispatcher (blocking until exit)
    view_dispatcher_run(app->view_dispatcher);
    
    // Cleanup
    battery_guardian_app_free(app);
    
    return 0;
}
