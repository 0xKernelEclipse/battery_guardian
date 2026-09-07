#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../core/telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

// Capability flags indicating charger hardware capabilities
#define CHARGER_CAP_NONE                (0)
#define CHARGER_CAP_CHARGE_CONTROL      (1 << 0) // Hardware charge stop/start control
#define CHARGER_CAP_CHARGE_STATUS       (1 << 1) // Reading charger status
#define CHARGER_CAP_BATTERY_VOLTAGE     (1 << 2) // Reading battery voltage
#define CHARGER_CAP_BATTERY_CURRENT     (1 << 3) // Reading battery current
#define CHARGER_CAP_BATTERY_TEMP        (1 << 4) // Reading battery temperature
#define CHARGER_CAP_BATTERY_SOC         (1 << 5) // Reading battery SOC %
#define CHARGER_CAP_GAUGE_STATUS        (1 << 6) // Reading fuel gauge OK state

// Abstract charger voltage limits that Flipper hardware can handle
typedef enum {
    ChargerLimit4_20V,
    ChargerLimit4_00V,
    ChargerLimit3_80V
} ChargerVoltageLimit;

// Unified HAL Boundary Interface
typedef struct {
    bool (*get_soc)(uint8_t* out_soc, void* context);
    bool (*get_voltage)(float* out_v, void* context);
    bool (*get_temperature)(float* out_temp, void* context);
    bool (*is_charging)(bool* out_charging, void* context);
    bool (*is_usb_present)(bool* out_usb, void* context);
    bool (*gauge_ok)(bool* out_ok, void* context);

    bool (*set_charge_suppressed)(bool suppress, void* context);
    uint32_t (*get_capabilities)(void* context);
    bool (*is_available)(void* context);
} ChargerHalInterface;

// Get active interface and context
const ChargerHalInterface* charger_hal_get_interface(void);
void* charger_hal_get_context(void);

// High-level HAL query and request wrappers
uint32_t charger_hal_get_capabilities(void);
bool charger_hal_is_available(void);
bool charger_hal_request_charge_enable(void);
bool charger_hal_request_charge_disable(void);

#ifdef BG_HOST_TEST
// Simulator mock helper state controls
void mock_charger_set_soc(uint8_t soc);
void mock_charger_set_voltage(float voltage);
void mock_charger_set_temperature(float temp);
void mock_charger_set_charging(bool charging);
void mock_charger_set_usb_present(bool present);
void mock_charger_set_gauge_ok(bool ok);
void mock_charger_set_cmd_rejection(bool reject);
void mock_charger_set_disappeared(bool disappeared);
void mock_charger_set_capabilities(uint32_t caps);

bool mock_charger_is_suppressed(void);
void mock_charger_reset(void);
#endif

#ifdef __cplusplus
}
#endif
