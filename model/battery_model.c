#include "battery_model.h"
#include "../phase2/battery_health.h"

struct BatteryModel {
    BatteryModelSnapshot snapshot;
    
    FuriMutex* mutex;
    FuriThread* thread;
    volatile bool running;
    
    HistoryModel* h_mod;
    SessionDataModel* s_mod;
    EventDataModel* e_mod;
    
    BatteryHealthEngine* health_engine;
    ChargePolicyEngine policy_engine;
    uint32_t last_processed_session_id;
};

static void battery_event_listener_cb(const BatteryEvent* event, void* context) {
    BatteryModel* model = context;
    diagnostics_inc_events_emitted();
    if(model->e_mod) {
        event_model_add_event(model->e_mod, event);
    }
}

static int32_t battery_model_worker_thread(void* context) {
    BatteryModel* model = context;
    
    while(model->running) {
        BatteryTelemetry sample;
        if(telemetry_take_sample(&sample)) {
            diagnostics_inc_samples_read();
            if(!(sample.flags & VALID_VOLTAGE) || !(sample.flags & VALID_TEMP) || !(sample.flags & VALID_SOC)) {
                diagnostics_inc_samples_invalid();
            }

            // Process core pipeline (NO MUTEX HELD)
            session_process_sample(&sample);
            if(journal_enqueue_sample(&sample)) {
                diagnostics_inc_journal_writes();
            } else {
                diagnostics_inc_journal_drops();
            }

            // Update Charge Policy Engine (Passive stub in production)
            const ChargerHalInterface* hal = charger_hal_get_interface();
            charge_policy_update(&model->policy_engine, hal, charger_hal_get_context(), sample.timestamp_ms);
            diagnostics_inc_policy_decisions();
            if(model->policy_engine.state == ChargeStateFault) {
                diagnostics_inc_safety_lockouts();
            }
            
            BatterySession current_session;
            bool has_session = session_get_active(&current_session);
            
            // Feed history and session caches
            if(model->h_mod) {
                history_model_add_sample(model->h_mod, &sample);
            }
            if(model->s_mod && has_session) {
                session_model_add_session(model->s_mod, &current_session);
            }
            
            // Feed phase 2 estimators
            BatterySession last_completed;
            if (session_get_last_completed(&last_completed)) {
                if (last_completed.session_id != model->last_processed_session_id) {
                    diagnostics_inc_sessions_completed();
                    EstimateRecordPayload new_estimate;
                    bool estimate_updated = battery_health_process_session(model->health_engine, &last_completed, &new_estimate);
                    model->last_processed_session_id = last_completed.session_id;
                    
                    if (estimate_updated) {
                        journal_write_record(RecordTypeEstimate, 1, last_completed.end_timestamp, &new_estimate, sizeof(EstimateRecordPayload));
                    }
                }
            }
            
            // Now acquire mutex to publish local dashboard snapshot
            furi_mutex_acquire(model->mutex, FuriWaitForever);
            
            memcpy(&model->snapshot.telemetry, &sample, sizeof(BatteryTelemetry));
            if(has_session) {
                memcpy(&model->snapshot.active_session, &current_session, sizeof(BatterySession));
            } else {
                model->snapshot.active_session.is_active = false;
            }
            model->snapshot.sample_count++;
            
            // Update Phase 2 snapshot
            battery_health_get_snapshot(model->health_engine, sample.timestamp_ms, &model->snapshot.health);
            
            // Copy charge policy and diagnostics to snapshot
            memcpy(&model->snapshot.policy_engine, &model->policy_engine, sizeof(ChargePolicyEngine));
            diagnostics_get_counters(&model->snapshot.diagnostics);
            
            furi_mutex_release(model->mutex);
        }
        
        furi_delay_ms(telemetry_get_interval_ms());
    }
    return 0;
}

// Callback for loading initial cache from journal
static bool battery_journal_load_cb(const JournalRecordHeader* header, const void* payload, void* context) {
    BatteryModel* model = context;
    if (header->type == RecordTypeSample) {
        if (model->h_mod) history_model_add_sample(model->h_mod, payload);
    } else if (header->type == RecordTypeSession) {
        if (model->s_mod) session_model_add_session(model->s_mod, payload);
        const BatterySession* s = payload;
        EstimateRecordPayload out;
        battery_health_process_session(model->health_engine, s, &out);
        model->last_processed_session_id = s->session_id;
    } else if (header->type == RecordTypeEvent) {
        if (model->e_mod) event_model_add_event(model->e_mod, payload);
    } else if (header->type == RecordTypeEstimate) {
        battery_health_load_estimate(model->health_engine, payload, header->timestamp);
    }
    return true; // continue
}

BatteryModel* battery_model_alloc(Storage* storage, HistoryModel* h_mod, SessionDataModel* s_mod, EventDataModel* e_mod) {
    BatteryModel* model = malloc(sizeof(BatteryModel));
    memset(model, 0, sizeof(BatteryModel));
    
    model->h_mod = h_mod;
    model->s_mod = s_mod;
    model->e_mod = e_mod;
    model->health_engine = battery_health_alloc();
    
    model->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    // Core pipeline init
    telemetry_init();
    session_manager_init();
    diagnostics_init();
    charge_policy_init(&model->policy_engine, ChargePolicyBalanced);
    
    // Listen for new events
    event_set_listener(battery_event_listener_cb, model);
    
    if (storage && storage_sd_status(storage) == FSE_OK) {
        model->snapshot.storage_ok = journal_init(storage, EXT_PATH("battery_guardian.log"));
        if(model->snapshot.storage_ok) {
            // Load caches from journal ONCE
            journal_iterate_records(storage, EXT_PATH("battery_guardian.log"), battery_journal_load_cb, model);
        }
    } else {
        model->snapshot.storage_ok = false;
    }
    
    model->thread = furi_thread_alloc();
    furi_thread_set_name(model->thread, "BatGuardModel");
    furi_thread_set_stack_size(model->thread, 2048); 
    furi_thread_set_context(model->thread, model);
    furi_thread_set_callback(model->thread, battery_model_worker_thread);
    
    return model;
}

void battery_model_free(BatteryModel* model) {
    battery_model_stop_polling(model);
    furi_thread_free(model->thread);
    furi_mutex_free(model->mutex);
    
    battery_health_free(model->health_engine);
    
    event_set_listener(NULL, NULL);
    journal_free();
    telemetry_free();
    diagnostics_free();
    
    free(model);
}

void battery_model_get_snapshot(BatteryModel* model, BatteryModelSnapshot* snapshot) {
    if(!model || !snapshot) return;
    if(furi_mutex_acquire(model->mutex, 100) == FuriStatusOk) {
        memcpy(snapshot, &model->snapshot, sizeof(BatteryModelSnapshot));
        furi_mutex_release(model->mutex);
    }
}

void battery_model_start_polling(BatteryModel* model) {
    if(!model->running) {
        model->running = true;
        furi_thread_start(model->thread);
    }
}

void battery_model_stop_polling(BatteryModel* model) {
    if(model->running) {
        model->running = false;
        furi_thread_join(model->thread);
    }
}
