#include "../test_helpers.h"
#include "../../core/event.h"
#include "../../core/telemetry.h"
#include "../mocks/furi.h"

static int g_event_callback_count = 0;
static BatteryEvent g_received_event;

static void test_event_cb(const BatteryEvent* event, void* context) {
    (void)context;
    g_received_event = *event;
    g_event_callback_count++;
}

void test_all_event_types_generation(void) {
    TEST_CASE("All required semantic event types generate valid snapshots and explanations");
    event_reset_state();
    event_set_listener(test_event_cb, NULL);
    
    BatteryTelemetry sample;
    memset(&sample, 0, sizeof(BatteryTelemetry));
    sample.timestamp_ms = 1000;
    sample.voltage_v = 4.10f;
    sample.current_a = 0.5f;
    sample.temperature_c = 32.0f;
    sample.soc_pct = 85;
    
    EventType all_types[] = {
        EventTypeChargeStarted,
        EventTypeChargeEnded,
        EventTypeChargeInterrupted,
        EventTypeDischargeStarted,
        EventTypeLowBattery,
        EventTypeCriticalBattery,
        EventTypeUsbConnected,
        EventTypeUsbDisconnected,
        EventTypeTemperatureSpike,
        EventTypeTemperatureRecovery,
        EventTypeVoltageAnomaly,
        EventTypeCurrentAnomaly,
        EventTypeGaugeError,
        EventTypeSocInconsistency,
        EventTypeCapacityEstimateUpdated,
        EventTypeCapacityEstimateRejected
    };
    
    size_t count = sizeof(all_types) / sizeof(all_types[0]);
    ASSERT_EQ(count, 16);
    
    for(size_t i = 0; i < count; i++) {
        g_event_callback_count = 0;
        sample.timestamp_ms += 10000; // ensure monotonic advancement
        
        char explanation[32];
        snprintf(explanation, sizeof(explanation), "Event test %zu", i);
        
        event_generate(all_types[i], EventSeverityInfo, explanation, &sample);
        
        ASSERT_EQ(g_event_callback_count, 1);
        ASSERT_EQ(g_received_event.type, all_types[i]);
        ASSERT_EQ(g_received_event.timestamp_ms, sample.timestamp_ms);
        ASSERT_STR_EQ(g_received_event.short_explanation, explanation);
        ASSERT_FLOAT_EQ(g_received_event.snapshot.voltage_v, 4.10f, 0.01f);
        ASSERT_EQ(g_received_event.snapshot.soc_pct, 85);
    }
    
    TEST_PASS();
}

void test_event_deduplication(void) {
    TEST_CASE("Duplicate events within debounce interval are suppressed");
    event_reset_state();
    event_set_listener(test_event_cb, NULL);
    
    BatteryTelemetry sample;
    memset(&sample, 0, sizeof(BatteryTelemetry));
    sample.timestamp_ms = 1000;
    
    // First event generated
    g_event_callback_count = 0;
    event_generate(EventTypeTemperatureSpike, EventSeverityWarning, "35C -> 42C", &sample);
    ASSERT_EQ(g_event_callback_count, 1);
    
    // Rapid duplicate 500ms later -> suppressed
    sample.timestamp_ms = 1500;
    event_generate(EventTypeTemperatureSpike, EventSeverityWarning, "35C -> 42C", &sample);
    ASSERT_EQ(g_event_callback_count, 1);
    
    // Event after debounce interval (6000ms later) -> allowed
    sample.timestamp_ms = 7500;
    event_generate(EventTypeTemperatureSpike, EventSeverityWarning, "35C -> 42C", &sample);
    ASSERT_EQ(g_event_callback_count, 2);
    
    TEST_PASS();
}

void run_event_generation_tests(void) {
    TEST_SUITE("Event Generation");
    test_all_event_types_generation();
    test_event_deduplication();
}
