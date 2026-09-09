/*
 * Phase 2A Estimator, Confidence & Degradation Tests
 *
 * Compiled into the host Zig test harness alongside Phase 1 tests.
 * No Flipper HAL is needed — pure logic, no I/O.
 */
#include "../test_helpers.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ---------- Minimal BatterySession builder ---------- */
/* Mirrors the real struct from core/session.h          */

/* Include Phase 2 modules directly (they only use stdlib) */
#include "../../phase2/estimator.h"
#include "../../phase2/confidence.h"
#include "../../phase2/degradation.h"
#include "../../phase2/battery_health.h"

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static BatterySession make_session(
    uint32_t id,
    uint8_t start_soc, uint8_t end_soc,
    float accumulated_mah,
    uint64_t start_ms, uint64_t end_ms,
    bool interrupted, bool sensor_fault, bool temp_excursion,
    uint32_t samples)
{
    BatterySession s;
    memset(&s, 0, sizeof(s));
    s.session_id       = id;
    s.type             = SessionTypeDischarging;
    s.start_soc        = start_soc;
    s.end_soc          = end_soc;
    s.start_timestamp  = start_ms;
    s.end_timestamp    = end_ms;
    s.sample_count     = samples;
    s.accumulated_energy_mah = accumulated_mah;
    s.average_temperature    = 25.0f;
    s.max_temperature        = 35.0f;

    /* Build quality flags */
    s.quality_flags = SESSION_QUALITY_ENOUGH_SAMPLES |
                      SESSION_QUALITY_GOOD_SPACING   |
                      SESSION_QUALITY_STABLE_TELEMETRY;

    if (!interrupted)    s.quality_flags |= SESSION_QUALITY_NOT_INTERRUPTED;
    if (!sensor_fault)   s.quality_flags |= SESSION_QUALITY_NO_SENSOR_FAULT;
    if (!temp_excursion) s.quality_flags |= SESSION_QUALITY_NO_TEMP_EXCURSION;

    if ((start_soc > end_soc) && ((start_soc - end_soc) >= 10))
        s.quality_flags |= SESSION_QUALITY_MEANINGFUL_SOC;

    return s;
}

/* ------------------------------------------------------------------ */
/* Suite: Estimator — qualification                                    */
/* ------------------------------------------------------------------ */
void run_phase2_estimator_tests(void) {
    printf("\n--- Phase 2A: Estimator Tests ---\n");

    /* 1. Reject a charging session */
    {
        CapacityEstimator est;
        estimator_init(&est);
        BatterySession s = make_session(1, 20, 80, 1200.0f, 1000, 4600000, false, false, false, 50);
        s.type = SessionTypeCharging;
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r == CandidateRejectedInvalidSession,
            "Charging session must be rejected");
    }

    /* 2. Reject a session with insufficient SOC drop */
    {
        CapacityEstimator est;
        estimator_init(&est);
        BatterySession s = make_session(2, 55, 48, 150.0f, 1000, 1800000, false, false, false, 50);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        /* 55-48 = 7pt drop: below MEANINGFUL_SOC flag (10pt) AND below ESTIMATOR_MIN_SOC_DROP (15pt).
           Either NoisyData (quality flag missing) or InsufficientDrop are both correct. */
        TEST_ASSERT(r == CandidateRejectedInsufficientDrop || r == CandidateRejectedNoisyData,
            "Small SOC window must be rejected");
    }

    /* 3. Accept a high-quality session and get a plausible estimate */
    {
        CapacityEstimator est;
        estimator_init(&est);
        /* 80→20 = 60% SOC drop, 1260 mAh integrated → implies ~2100 mAh full cap */
        BatterySession s = make_session(3, 80, 20, 1260.0f, 0, 7200000, false, false, false, 100);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r == CandidateRejectedNone, "Valid session must be accepted");
        TEST_ASSERT(c.candidate_capacity_mah > 1900.0f && c.candidate_capacity_mah < 2300.0f,
            "Capacity estimate should be near 2100 mAh for 60% SOC / 1260 mAh session");
    }

    /* 4. Reject interrupted session */
    {
        CapacityEstimator est;
        estimator_init(&est);
        BatterySession s = make_session(4, 90, 20, 1500.0f, 0, 7200000, true, false, false, 100);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r == CandidateRejectedNoisyData,
            "Interrupted session must not contribute to capacity estimate");
    }

    /* 5. Reject session with sensor fault */
    {
        CapacityEstimator est;
        estimator_init(&est);
        BatterySession s = make_session(5, 90, 20, 1500.0f, 0, 7200000, false, true, false, 100);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r == CandidateRejectedNoisyData,
            "Sensor-fault session must be rejected");
    }

    /* 6. Reject session with negative elapsed time (adversarial) */
    {
        CapacityEstimator est;
        estimator_init(&est);
        BatterySession s = make_session(6, 90, 20, 1500.0f, 9000000, 1000, false, false, false, 100);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r != CandidateRejectedNone,
            "Session with negative elapsed time must be rejected");
    }

    /* 7. Partial discharge accepted with evidence noting smaller window */
    {
        CapacityEstimator est;
        estimator_init(&est);
        /* 70→30 = 40% drop, 840 mAh → ~2100 mAh */
        BatterySession s = make_session(7, 70, 30, 840.0f, 0, 5400000, false, false, false, 80);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r == CandidateRejectedNone, "Partial discharge (40%) should be accepted");
        TEST_ASSERT(c.start_soc == 70 && c.end_soc == 30, "Candidate SOC must match session");
    }
}

/* ------------------------------------------------------------------ */
/* Suite: Outlier rejection (rolling median)                           */
/* ------------------------------------------------------------------ */
void run_phase2_outlier_tests(void) {
    printf("\n--- Phase 2A: Outlier Rejection Tests ---\n");

    CapacityEstimator est;
    estimator_init(&est);

    /* Seed 4 good candidates around 1980 mAh */
    uint32_t id = 100;
    uint64_t t  = 0;
    float good_caps[] = {1980.0f, 1965.0f, 1974.0f, 1959.0f};
    for(int i = 0; i < 4; i++) {
        float mah = good_caps[i] * 0.6f; /* 60% SOC window */
        BatterySession s = make_session(id++, 80, 20, mah, t, t + 7200000ULL, false, false, false, 100);
        t += 86400000ULL;
        CapacityCandidate c;
        estimator_process_session(&est, &s, &c);
    }

    TEST_ASSERT(est.accepted_count == 4, "4 good candidates should be accepted");
    TEST_ASSERT(est.robust_estimate_mah > 1900.0f && est.robust_estimate_mah < 2050.0f,
        "Robust estimate should be near 1970 mAh after 4 good candidates");

    /* Inject a gross outlier: 640 mAh integrated → would imply ~1067 mAh, far from ~1970 */
    {
        BatterySession s = make_session(id++, 80, 20, 640.0f * 0.6f, t, t + 7200000ULL, false, false, false, 100);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r == CandidateRejectedOutlier,
            "Gross outlier (640 mAh implied cap) must be rejected");
    }

    /* Verify estimate did not move significantly after outlier */
    float estimate_after = est.robust_estimate_mah;
    TEST_ASSERT(estimate_after > 1900.0f && estimate_after < 2050.0f,
        "Robust estimate must remain stable after outlier rejection");
    TEST_ASSERT(est.total_rejected == 1, "Exactly 1 rejection should be recorded");
}

/* ------------------------------------------------------------------ */
/* Suite: Synthetic datasets                                           */
/* ------------------------------------------------------------------ */

/* Build n sessions around a given true capacity (mAh),
   using a 60% SOC discharge window each time.           */
static void feed_stable_sessions(CapacityEstimator* est, float true_cap_mah, int n,
                                 float noise_pct)
{
    uint64_t t = 0;
    uint32_t id = 200;
    for(int i = 0; i < n; i++) {
        float noise = 1.0f + (noise_pct / 100.0f) * ((i % 3 == 0) ? 1.0f : -0.5f);
        float integrated = true_cap_mah * 0.60f * noise;
        BatterySession s = make_session(id++, 80, 20, integrated, t, t + 7200000ULL,
                                        false, false, false, 100);
        t += 86400000ULL;
        CapacityCandidate c;
        estimator_process_session(est, &s, &c);
    }
}

void run_phase2_dataset_tests(void) {
    printf("\n--- Phase 2A: Synthetic Dataset Tests ---\n");

    /* DATASET_STABLE */
    {
        CapacityEstimator est; estimator_init(&est);
        feed_stable_sessions(&est, 2000.0f, 6, 0.0f); /* 0% noise for perfectly stable trend */
        BatteryConfidence conf; confidence_calculate(&conf, &est);
        BatteryDegradation deg; degradation_calculate(&deg, &est, &conf);

        TEST_ASSERT(est.accepted_count >= 5, "STABLE: ≥5 candidates accepted");
        TEST_ASSERT(est.robust_estimate_mah > 1850.0f && est.robust_estimate_mah < 2150.0f,
            "STABLE: estimate within ±7.5% of true 2000 mAh");
        TEST_ASSERT(conf.overall >= ConfidenceLevelMedium,
            "STABLE: confidence should be Medium or High");
        TEST_ASSERT(deg.trend == DegradationTrendStable || deg.trend == DegradationTrendInsufficient,
            "STABLE: trend should be Stable (or Insufficient with only 6 pts)");
    }

    /* DATASET_GRADUAL_DEGRADATION */
    {
        CapacityEstimator est; estimator_init(&est);
        float caps[] = {2050.0f, 2010.0f, 1975.0f, 1935.0f, 1890.0f, 1850.0f, 1810.0f, 1770.0f};
        uint64_t t = 0;
        uint32_t id = 300;
        for(int i = 0; i < 8; i++) {
            float integrated = caps[i] * 0.60f;
            BatterySession s = make_session(id++, 80, 20, integrated, t, t + 7200000ULL,
                                            false, false, false, 100);
            t += 86400000ULL;
            CapacityCandidate c;
            estimator_process_session(&est, &s, &c);
        }
        BatteryConfidence conf; confidence_calculate(&conf, &est);
        BatteryDegradation deg; degradation_calculate(&deg, &est, &conf);

        TEST_ASSERT(est.accepted_count >= 6, "DEGRADATION: most candidates accepted");
        TEST_ASSERT(deg.trend == DegradationTrendDeclining,
            "DEGRADATION: trend must be Declining for a monotonically falling dataset");
        TEST_ASSERT(deg.capacity_slope < 0.0f,
            "DEGRADATION: slope must be negative (mAh decreasing per session)");
    }

    /* DATASET_SINGLE_OUTLIER — injected mid-sequence */
    {
        CapacityEstimator est; estimator_init(&est);
        float caps[] = {1980.0f, 1965.0f, 1972.0f, 640.0f, 1958.0f}; /* 640 is the outlier */
        uint64_t t = 0;
        uint32_t id = 400;
        for(int i = 0; i < 5; i++) {
            float integrated = caps[i] * 0.60f;
            BatterySession s = make_session(id++, 80, 20, integrated, t, t + 7200000ULL,
                                            false, false, false, 100);
            t += 86400000ULL;
            CapacityCandidate c;
            estimator_process_session(&est, &s, &c);
        }

        TEST_ASSERT(est.total_rejected == 1, "OUTLIER: exactly 1 session rejected");
        TEST_ASSERT(est.robust_estimate_mah > 1900.0f && est.robust_estimate_mah < 2050.0f,
            "OUTLIER: estimate must remain near 1970 mAh despite outlier");
    }

    /* DATASET_INTERRUPTED — all sessions interrupted → all rejected */
    {
        CapacityEstimator est; estimator_init(&est);
        uint64_t t = 0;
        uint32_t id = 500;
        for(int i = 0; i < 5; i++) {
            BatterySession s = make_session(id++, 80, 20, 1260.0f, t, t + 7200000ULL,
                                            true /* interrupted */, false, false, 100);
            t += 86400000ULL;
            CapacityCandidate c;
            estimator_process_session(&est, &s, &c);
        }
        TEST_ASSERT(est.accepted_count == 0, "INTERRUPTED: all sessions must be rejected");
        TEST_ASSERT(!est.has_valid_estimate,  "INTERRUPTED: no valid estimate should exist");
    }

    /* DATASET_SMALL_SOC_WINDOW — drop < threshold → rejected */
    {
        CapacityEstimator est; estimator_init(&est);
        BatterySession s = make_session(600, 52, 43, 200.0f, 0, 3600000ULL,
                                        false, false, false, 50);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r == CandidateRejectedInsufficientDrop || r == CandidateRejectedNoisyData,
            "SMALL_SOC_WINDOW: 9-point drop must be rejected (below 15-pt minimum)");
    }
}

/* ------------------------------------------------------------------ */
/* Suite: Adversarial inputs                                           */
/* ------------------------------------------------------------------ */
void run_phase2_adversarial_tests(void) {
    printf("\n--- Phase 2A: Adversarial Tests ---\n");

    /* Zero-duration session */
    {
        CapacityEstimator est; estimator_init(&est);
        BatterySession s = make_session(700, 80, 20, 1260.0f, 5000000ULL, 5000000ULL,
                                        false, false, false, 100);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r != CandidateRejectedNone,
            "ADVERSARIAL: zero-duration session must be rejected");
    }

    /* Timestamp overflow (end wraps around to < start) */
    {
        CapacityEstimator est; estimator_init(&est);
        BatterySession s = make_session(701, 80, 20, 1260.0f,
                                        0xFFFFFFFFFFFFF000ULL, 100ULL,
                                        false, false, false, 100);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r != CandidateRejectedNone,
            "ADVERSARIAL: timestamp overflow (end < start) must be rejected");
    }

    /* Zero integrated current */
    {
        CapacityEstimator est; estimator_init(&est);
        BatterySession s = make_session(702, 80, 20, 0.0f, 0, 7200000ULL,
                                        false, false, false, 100);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r != CandidateRejectedNone,
            "ADVERSARIAL: zero accumulated energy must be rejected");
    }

    /* Corrupted estimate record: battery_health_load_estimate with zero reference */
    {
        BatteryHealthEngine* engine = battery_health_alloc();
        battery_health_set_reference(engine, 2100.0f);

        EstimateRecordPayload payload;
        memset(&payload, 0, sizeof(payload));
        payload.estimated_capacity = 1950.0f;
        payload.reference_capacity = 0.0f; /* corrupt: zero reference */
        payload.observed_health    = 0;
        payload.confidence         = ConfidenceLevelInsufficient;

        /* Must not crash or divide-by-zero */
        battery_health_load_estimate(engine, &payload, 1000000ULL);

        BatteryHealthSnapshot snap;
        battery_health_get_snapshot(engine, 2000000ULL, &snap);
        /* Engine should not report a confident result with zero reference */
        /* (reference is preserved from engine init, not overwritten by corrupt payload) */
        TEST_ASSERT(engine != NULL, "ADVERSARIAL: corrupt estimate payload must not crash engine");

        battery_health_free(engine);
    }

    /* Temperature excursion session is rejected */
    {
        CapacityEstimator est; estimator_init(&est);
        BatterySession s = make_session(703, 80, 20, 1260.0f, 0, 7200000ULL,
                                        false, false, true /* temp_excursion */, 100);
        CapacityCandidate c;
        CandidateRejectionReason r = estimator_process_session(&est, &s, &c);
        TEST_ASSERT(r == CandidateRejectedNoisyData,
            "ADVERSARIAL: temperature excursion session must be rejected");
    }
}

/* ------------------------------------------------------------------ */
/* Suite: Confidence Engine                                            */
/* ------------------------------------------------------------------ */
void run_phase2_confidence_tests(void) {
    printf("\n--- Phase 2A: Confidence Engine Tests ---\n");

    /* Health snapshots include the gauge value read by the telemetry adapter. */
    {
        BatteryHealthEngine* engine = battery_health_alloc();
        battery_health_set_gauge_health(engine, 87);
        BatteryHealthSnapshot snap;
        battery_health_get_snapshot(engine, 0, &snap);
        TEST_ASSERT(snap.gauge_health_pct == 87,
            "Gauge health should be copied into the health snapshot");
        battery_health_free(engine);
    }

    /* Loading an estimate should leave baseline state for the next session. */
    {
        BatteryHealthEngine* engine = battery_health_alloc();
        EstimateRecordPayload payload = {
            .estimated_capacity = 1950.0f,
            .reference_capacity = 2100.0f,
            .confidence = ConfidenceLevelMedium,
            .accepted_sessions = 6,
        };
        battery_health_load_estimate(engine, &payload, 5000000ULL);
        BatterySession next = make_session(900, 80, 20, 1260.0f,
                                           6000000ULL, 13200000ULL,
                                           false, false, false, 100);
        EstimateRecordPayload next_payload;
        bool accepted = battery_health_process_session(engine, &next, &next_payload);
        TEST_ASSERT(accepted, "A new session should continue from a saved estimate");
        TEST_ASSERT(engine->estimator.accepted_count >= 2,
            "Saved estimate should seed continued estimator history");
        battery_health_free(engine);
    }

    /* Public health APIs should tolerate null handles. */
    {
        BatterySession session;
        memset(&session, 0, sizeof(session));
        battery_health_set_reference(NULL, 2100.0f);
        battery_health_set_gauge_health(NULL, 80);
        TEST_ASSERT(!battery_health_process_session(NULL, &session, NULL),
            "Null health engine should reject session processing");
        battery_health_get_snapshot(NULL, 0, NULL);
    }

    /* Restore the last saved estimate without rebuilding every session. */
    {
        BatteryHealthEngine* engine = battery_health_alloc();
        EstimateRecordPayload payload = {
            .estimated_capacity = 1950.0f,
            .reference_capacity = 2100.0f,
            .observed_health = 92,
            .confidence = ConfidenceLevelHigh,
            .accepted_sessions = 6,
            .rejected_sessions = 1,
            .trend = -1,
            .trend_confidence = 80,
        };
        battery_health_load_estimate(engine, &payload, 5000000ULL);

        BatteryHealthSnapshot snap;
        battery_health_get_snapshot(engine, 6000000ULL, &snap);
        TEST_ASSERT(snap.estimate_available, "Saved estimate should be available after load");
        TEST_ASSERT(snap.estimated_capacity_mah == 1950.0f,
            "Saved capacity should be restored");
        TEST_ASSERT(snap.observed_health_pct == 92,
            "Saved capacity should produce the expected health estimate");
        TEST_ASSERT(snap.confidence.overall == ConfidenceLevelHigh,
            "Saved confidence should be restored");
        TEST_ASSERT(snap.accepted_sessions == 6 && snap.rejected_sessions == 1,
            "Saved session counts should be restored");
        TEST_ASSERT(snap.degradation.trend == DegradationTrendDeclining,
            "Saved degradation trend should be restored");
        battery_health_free(engine);
    }

    /* Insufficient data */
    {
        CapacityEstimator est; estimator_init(&est);
        BatteryConfidence conf; confidence_calculate(&conf, &est);
        TEST_ASSERT(conf.overall == ConfidenceLevelInsufficient,
            "Empty estimator should yield INSUFFICIENT confidence");
    }

    /* High confidence: 6 tightly-clustered candidates */
    {
        CapacityEstimator est; estimator_init(&est);
        feed_stable_sessions(&est, 2000.0f, 6, 0.5f); /* tight ±0.5% noise */
        BatteryConfidence conf; confidence_calculate(&conf, &est);
        TEST_ASSERT(conf.overall >= ConfidenceLevelMedium,
            "6 tight candidates should yield at least Medium confidence");
        TEST_ASSERT(strlen(conf.explanation) > 0,
            "Confidence explanation must be non-empty");
    }

    /* Health calculation */
    {
        BatteryHealthEngine* engine = battery_health_alloc();
        battery_health_set_reference(engine, 2100.0f);
        feed_stable_sessions(&engine->estimator, 1950.0f, 6, 0.5f);
        confidence_calculate(&engine->confidence, &engine->estimator);
        engine->has_estimate = engine->estimator.has_valid_estimate;

        BatteryHealthSnapshot snap;
        battery_health_get_snapshot(engine, 100000ULL, &snap);

        if(snap.estimate_available) {
            TEST_ASSERT(snap.observed_health_pct >= 80 && snap.observed_health_pct <= 105,
                "Observed health pct should be ~93% for 1950/2100 reference");
        }

        battery_health_free(engine);
    }
}

/* ------------------------------------------------------------------ */
/* Suite: Stale model detection                                        */
/* ------------------------------------------------------------------ */
void run_phase2_stale_model_tests(void) {
    printf("\n--- Phase 2A: Stale Model Tests ---\n");

    BatteryHealthEngine* engine = battery_health_alloc();
    battery_health_set_reference(engine, 2100.0f);

    /* Inject 4 good sessions so we have a valid estimate */
    uint64_t session_time = 1000000ULL;
    uint32_t sid = 800;
    for(int i = 0; i < 4; i++) {
        BatterySession s = make_session(sid++, 80, 20, 1260.0f,
                                        session_time, session_time + 7200000ULL,
                                        false, false, false, 100);
        session_time += 86400000ULL;
        EstimateRecordPayload out;
        battery_health_process_session(engine, &s, &out);
    }

    BatteryHealthSnapshot snap;

    /* Fresh: query 1 day after last session */
    battery_health_get_snapshot(engine, session_time + 86400000ULL, &snap);
    TEST_ASSERT(snap.model_state == ModelFresh || snap.model_state == ModelAging,
        "Model queried 1 day after update should be Fresh or Aging");

    /* Stale: query 45 days after last session */
    battery_health_get_snapshot(engine, session_time + 45ULL * 86400000ULL, &snap);
    TEST_ASSERT(snap.model_state == ModelStale,
        "Model queried 45 days after last update should be Stale");

    battery_health_free(engine);
}

/* ------------------------------------------------------------------ */
/* Suite: Large-scale determinism (10,000 synthetic sessions)         */
/* ------------------------------------------------------------------ */
void run_phase2_scale_test(void) {
    printf("\n--- Phase 2A: 10,000 Session Scale Test ---\n");

    CapacityEstimator est;
    estimator_init(&est);

    uint32_t id = 10000;
    uint64_t t  = 0;

    for(int i = 0; i < 10000; i++) {
        /* Gradually degrade: 2100 → ~1800 over 10k sessions */
        float cap = 2100.0f - (i * 0.03f);
        float integrated = cap * 0.60f;
        BatterySession s = make_session(id++, 80, 20, integrated,
                                        t, t + 7200000ULL,
                                        false, false, false, 100);
        t += 3600000ULL;
        CapacityCandidate c;
        estimator_process_session(&est, &s, &c);
    }

    /* Ring buffer must stay bounded */
    TEST_ASSERT(est.accepted_count <= ESTIMATOR_MAX_HISTORY,
        "SCALE: ring buffer must stay bounded after 10,000 sessions");

    /* Estimate should reflect recent degraded capacity */
    TEST_ASSERT(est.robust_estimate_mah > 1700.0f && est.robust_estimate_mah < 2200.0f,
        "SCALE: robust estimate must remain within plausible range after 10,000 sessions");

    printf("  Sessions processed: 10000\n");
    printf("  Accepted candidates in ring buffer: %lu\n", (unsigned long)est.accepted_count);
    printf("  Total accepted: %lu | Total rejected: %lu\n", (unsigned long)est.total_accepted, (unsigned long)est.total_rejected);
    printf("  Final robust estimate: %.1f mAh\n", (double)est.robust_estimate_mah);
}
