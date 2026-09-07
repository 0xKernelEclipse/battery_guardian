#pragma once
#include <stdio.h>
#include "../../test_helpers.h"
// Capacity tolerance: ±1%
#define SIM_TOLERANCE_CAPACITY_PCT 1.0f

// Health tolerance: ±1 percentage point
#define SIM_TOLERANCE_HEALTH_PTS 1.0f

// Helper macro for float equality with tolerance
#define SIM_ASSERT_FLOAT_NEAR(actual, expected, tolerance) \
    do { \
        float diff = ((actual) > (expected)) ? ((actual) - (expected)) : ((expected) - (actual)); \
        if (diff > (tolerance)) { \
            printf("  [FAIL] Expected near %.2f (±%.2f), got %.2f (%s:%d)\n", \
                   (double)(expected), (double)(tolerance), (double)(actual), __FILE__, __LINE__); \
            g_tests_failed++; \
        } else { \
            printf("  [PASS] Value %.2f is within ±%.2f of %.2f\n", \
                   (double)(actual), (double)(tolerance), (double)(expected)); \
            g_tests_passed++; \
        } \
        g_tests_run++; \
        fflush(stdout); \
    } while(0)
