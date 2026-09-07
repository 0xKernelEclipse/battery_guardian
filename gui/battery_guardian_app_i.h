#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include <gui/modules/dialog_ex.h>
#include <storage/storage.h>

#include "../model/battery_model.h"
#include "../model/history_model.h"
#include "../model/session_model.h"
#include "../model/event_model.h"

// Forward declarations
typedef struct BatteryGuardianApp BatteryGuardianApp;

// View enums
typedef enum {
    BatteryGuardianViewDashboard,
    BatteryGuardianViewHistory,
    BatteryGuardianViewSessions,
    BatteryGuardianViewDiagnostics,
    BatteryGuardianViewEvents,
    BatteryGuardianViewMenu,
    BatteryGuardianViewHealth,
    BatteryGuardianViewExplanation,
} BatteryGuardianView;

// Custom Views
#include "views/dashboard.h"
#include "views/history.h"
#include "views/sessions.h"
#include "views/diagnostics.h"
#include "views/events.h"
#include "views/health.h"
#include "views/explanation.h"

struct BatteryGuardianApp {
    Gui* gui;
    Storage* storage;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    // Core Models
    BatteryModel* battery_model;
    HistoryModel* history_model;
    SessionDataModel* session_model;
    EventDataModel* event_model;

    FuriTimer* update_timer; // Timer to refresh UI

    // Common GUI modules
    Submenu* submenu;
    Widget* widget;
    DialogEx* dialog;

    // Custom views
    DashboardView* dashboard;
    HistoryView* history;
    SessionsView* sessions;
    DiagnosticsView* diagnostics;
    EventsView* events;
    HealthView* health;
    ExplanationView* explanation;
};

// Scene enums and events
typedef enum {
    EventDashboardToHistory = 100,
    EventDashboardToSessions,
    EventDashboardToMenu,
    EventDashboardToHealth,
} CustomEvent;

