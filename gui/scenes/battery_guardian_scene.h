#pragma once

#include <gui/scene_manager.h>

// Generate scene id and total number
#define ADD_SCENE(prefix, name, id) BatteryGuardianScene##id,
typedef enum {
#include "battery_guardian_scene_config.h"
    BatteryGuardianSceneNum,
} BatteryGuardianScene;
#undef ADD_SCENE

extern const SceneManagerHandlers battery_guardian_scene_handlers;

// Scene on_enter handlers
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_enter(void*);
#include "battery_guardian_scene_config.h"
#undef ADD_SCENE

// Scene on_event handlers
#define ADD_SCENE(prefix, name, id) \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent event);
#include "battery_guardian_scene_config.h"
#undef ADD_SCENE

// Scene on_exit handlers
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_exit(void* context);
#include "battery_guardian_scene_config.h"
#undef ADD_SCENE
