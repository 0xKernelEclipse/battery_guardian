#include "../phase2/charger_hal.h"
#include <string.h>

#ifdef BG_HOST_TEST
// Host testing mock state
static uint8_t m_soc = 50;
static float m_voltage = 3.8f;
static float m_temp = 25.0f;
static bool m_charging = true;
static bool m_usb_present = true;
static bool m_gauge_ok = true;
static bool m_suppressed = false;

static bool m_reject_commands = false;
static bool m_disappeared = false;
static uint32_t m_capabilities = 
    CHARGER_CAP_CHARGE_CONTROL | 
    CHARGER_CAP_CHARGE_STATUS | 
    CHARGER_CAP_BATTERY_VOLTAGE | 
    CHARGER_CAP_BATTERY_CURRENT | 
    CHARGER_CAP_BATTERY_TEMP | 
    CHARGER_CAP_BATTERY_SOC | 
    CHARGER_CAP_GAUGE_STATUS;

void mock_charger_set_soc(uint8_t soc) { m_soc = soc; }
void mock_charger_set_voltage(float voltage) { m_voltage = voltage; }
void mock_charger_set_temperature(float temp) { m_temp = temp; }
void mock_charger_set_charging(bool charging) { m_charging = charging; }
void mock_charger_set_usb_present(bool present) { m_usb_present = present; }
void mock_charger_set_gauge_ok(bool ok) { m_gauge_ok = ok; }
void mock_charger_set_cmd_rejection(bool reject) { m_reject_commands = reject; }
void mock_charger_set_disappeared(bool disappeared) { m_disappeared = disappeared; }
void mock_charger_set_capabilities(uint32_t caps) { m_capabilities = caps; }
bool mock_charger_is_suppressed(void) { return m_suppressed; }

void mock_charger_reset(void) {
    m_soc = 50;
    m_voltage = 3.8f;
    m_temp = 25.0f;
    m_charging = true;
    m_usb_present = true;
    m_gauge_ok = true;
    m_suppressed = false;
    m_reject_commands = false;
    m_disappeared = false;
    m_capabilities = 
        CHARGER_CAP_CHARGE_CONTROL | 
        CHARGER_CAP_CHARGE_STATUS | 
        CHARGER_CAP_BATTERY_VOLTAGE | 
        CHARGER_CAP_BATTERY_CURRENT | 
        CHARGER_CAP_BATTERY_TEMP | 
        CHARGER_CAP_BATTERY_SOC | 
        CHARGER_CAP_GAUGE_STATUS;
}

// Interface methods
static bool mock_get_soc(uint8_t* out_soc, void* context) {
    (void)context;
    if (m_disappeared || !(m_capabilities & CHARGER_CAP_BATTERY_SOC)) return false;
    *out_soc = m_soc;
    return true;
}

static bool mock_get_voltage(float* out_v, void* context) {
    (void)context;
    if (m_disappeared || !(m_capabilities & CHARGER_CAP_BATTERY_VOLTAGE)) return false;
    *out_v = m_voltage;
    return true;
}

static bool mock_get_temperature(float* out_temp, void* context) {
    (void)context;
    if (m_disappeared || !(m_capabilities & CHARGER_CAP_BATTERY_TEMP)) return false;
    *out_temp = m_temp;
    return true;
}

static bool mock_is_charging(bool* out_charging, void* context) {
    (void)context;
    if (m_disappeared || !(m_capabilities & CHARGER_CAP_CHARGE_STATUS)) return false;
    *out_charging = m_charging;
    return true;
}

static bool mock_is_usb_present(bool* out_usb, void* context) {
    (void)context;
    if (m_disappeared) return false;
    *out_usb = m_usb_present;
    return true;
}

static bool mock_gauge_ok(bool* out_ok, void* context) {
    (void)context;
    if (m_disappeared || !(m_capabilities & CHARGER_CAP_GAUGE_STATUS)) return false;
    *out_ok = m_gauge_ok;
    return true;
}

static bool mock_set_charge_suppressed(bool suppress, void* context) {
    (void)context;
    if (m_disappeared || m_reject_commands || !(m_capabilities & CHARGER_CAP_CHARGE_CONTROL)) {
        return false;
    }
    m_suppressed = suppress;
    if (suppress) {
        m_charging = false;
    } else {
        m_charging = m_usb_present;
    }
    return true;
}

static uint32_t mock_get_capabilities(void* context) {
    (void)context;
    if (m_disappeared) return CHARGER_CAP_NONE;
    return m_capabilities;
}

static bool mock_is_available(void* context) {
    (void)context;
    return !m_disappeared;
}

static const ChargerHalInterface mock_interface = {
    .get_soc = mock_get_soc,
    .get_voltage = mock_get_voltage,
    .get_temperature = mock_get_temperature,
    .is_charging = mock_is_charging,
    .is_usb_present = mock_is_usb_present,
    .gauge_ok = mock_gauge_ok,
    .set_charge_suppressed = mock_set_charge_suppressed,
    .get_capabilities = mock_get_capabilities,
    .is_available = mock_is_available,
};

const ChargerHalInterface* charger_hal_get_interface(void) {
    return &mock_interface;
}

void* charger_hal_get_context(void) {
    return NULL;
}

#else
// Production Flipper FAP: Strictly PASSIVE and FAIL-CLOSED (Zero hardware register writes)
#include <furi.h>
#include <furi_hal_power.h>

static bool prod_get_soc(uint8_t* out_soc, void* context) {
    (void)context;
    *out_soc = furi_hal_power_get_pct();
    return true;
}

static bool prod_get_voltage(float* out_v, void* context) {
    (void)context;
    *out_v = furi_hal_power_get_battery_voltage(FuriHalPowerICFuelGauge);
    return true;
}

static bool prod_get_temperature(float* out_temp, void* context) {
    (void)context;
    *out_temp = furi_hal_power_get_battery_temperature(FuriHalPowerICFuelGauge);
    return true;
}

static bool prod_is_charging(bool* out_charging, void* context) {
    (void)context;
    *out_charging = furi_hal_power_is_charging();
    return true;
}

static bool prod_is_usb_present(bool* out_usb, void* context) {
    (void)context;
    *out_usb = furi_hal_power_is_charging() || 
               (furi_hal_power_get_usb_voltage() > 4.0f) || 
               furi_hal_power_is_otg_enabled();
    return true;
}

static bool prod_gauge_ok(bool* out_ok, void* context) {
    (void)context;
    *out_ok = furi_hal_power_gauge_is_ok();
    return true;
}

// In production, charge control is deliberately disabled (FAIL-CLOSED) to prevent
// untested physical manipulation of the BQ25896 charger PMIC registers.
static bool prod_set_charge_suppressed(bool suppress, void* context) {
    (void)context;
    (void)suppress;
    FURI_LOG_W("BatGuard", "PASSIVE FAIL-CLOSED: Physical charge suppression rejected on hardware.");
    return false; // Safely refuse hardware manipulation
}

static uint32_t prod_get_capabilities(void* context) {
    (void)context;
    // Charge control bit is omitted (passive telemetry only)
    return CHARGER_CAP_CHARGE_STATUS | 
           CHARGER_CAP_BATTERY_VOLTAGE | 
           CHARGER_CAP_BATTERY_CURRENT | 
           CHARGER_CAP_BATTERY_TEMP | 
           CHARGER_CAP_BATTERY_SOC | 
           CHARGER_CAP_GAUGE_STATUS;
}

static bool prod_is_available(void* context) {
    (void)context;
    return furi_hal_power_gauge_is_ok();
}

static const ChargerHalInterface prod_interface = {
    .get_soc = prod_get_soc,
    .get_voltage = prod_get_voltage,
    .get_temperature = prod_get_temperature,
    .is_charging = prod_is_charging,
    .is_usb_present = prod_is_usb_present,
    .gauge_ok = prod_gauge_ok,
    .set_charge_suppressed = prod_set_charge_suppressed,
    .get_capabilities = prod_get_capabilities,
    .is_available = prod_is_available,
};

const ChargerHalInterface* charger_hal_get_interface(void) {
    return &prod_interface;
}

void* charger_hal_get_context(void) {
    return NULL;
}

#endif

// High-level HAL query and request functions
uint32_t charger_hal_get_capabilities(void) {
    const ChargerHalInterface* iface = charger_hal_get_interface();
    if (!iface || !iface->get_capabilities) return CHARGER_CAP_NONE;
    return iface->get_capabilities(charger_hal_get_context());
}

bool charger_hal_is_available(void) {
    const ChargerHalInterface* iface = charger_hal_get_interface();
    if (!iface || !iface->is_available) return false;
    return iface->is_available(charger_hal_get_context());
}

bool charger_hal_request_charge_enable(void) {
    const ChargerHalInterface* iface = charger_hal_get_interface();
    if (!iface || !iface->set_charge_suppressed) return false;
    return iface->set_charge_suppressed(false, charger_hal_get_context());
}

bool charger_hal_request_charge_disable(void) {
    const ChargerHalInterface* iface = charger_hal_get_interface();
    if (!iface || !iface->set_charge_suppressed) return false;
    return iface->set_charge_suppressed(true, charger_hal_get_context());
}
