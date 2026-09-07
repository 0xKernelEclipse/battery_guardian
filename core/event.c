#include "event.h"
#include "../storage/journal.h"
#include <string.h>

static BatteryEvent last_event;
static bool has_last_event = false;
static EventListenerCallback event_listener = NULL;
static void* event_listener_context = NULL;

void event_set_listener(EventListenerCallback callback, void* context) {
    event_listener = callback;
    event_listener_context = context;
}

void event_reset_state(void) {
    memset(&last_event, 0, sizeof(BatteryEvent));
    has_last_event = false;
    event_listener = NULL;
    event_listener_context = NULL;
}

void event_generate(EventType type, EventSeverity severity, const char* explanation, const BatteryTelemetry* snapshot) {
    if (!snapshot) return;

    // Check de-duplication for certain events if identical to last event within short window (e.g. 5 seconds)
    if(has_last_event && last_event.type == type && (snapshot->timestamp_ms - last_event.timestamp_ms < 5000)) {
        return;
    }

    BatteryEvent event;
    memset(&event, 0, sizeof(BatteryEvent));
    event.timestamp_ms = snapshot->timestamp_ms;
    event.type = type;
    event.severity = severity;
    
    if (explanation) {
        strncpy(event.short_explanation, explanation, sizeof(event.short_explanation) - 1);
    }
    
    memcpy(&event.snapshot, snapshot, sizeof(BatteryTelemetry));
    last_event = event;
    has_last_event = true;

    // Notify listener if registered
    if(event_listener) {
        event_listener(&event, event_listener_context);
    }

    // Persist event immediately via journal (if journal initialized)
    journal_write_record(RecordTypeEvent, EVENT_PAYLOAD_VERSION, event.timestamp_ms, &event, sizeof(BatteryEvent));
}

bool event_get_last(BatteryEvent* out_event) {
    if(!has_last_event) return false;
    if(out_event) {
        *out_event = last_event;
    }
    return true;
}
