#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

// Global test counters
extern int g_tests_run;
extern int g_tests_passed;
extern int g_tests_failed;

#define TEST_SUITE(name) \
    printf("\n=== Running Suite: %s ===\n", name)

#define TEST_CASE(name) \
    do { \
        printf("  [TEST] %s ... ", name); \
        g_tests_run++; \
        fflush(stdout); \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASS\n"); \
        g_tests_passed++; \
        fflush(stdout); \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf("FAIL (%s:%d: %s)\n", __FILE__, __LINE__, msg); \
        g_tests_failed++; \
        fflush(stdout); \
        return; \
    } while(0)

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            TEST_FAIL("Expected condition '" #cond "' to be true"); \
        } \
    } while(0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            TEST_FAIL("Expected condition '" #cond "' to be false"); \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            char _err_buf[128]; \
            snprintf(_err_buf, sizeof(_err_buf), "Expected %ld == %ld", (long)(a), (long)(b)); \
            TEST_FAIL(_err_buf); \
        } \
    } while(0)

#define ASSERT_FLOAT_EQ(a, b, epsilon) \
    do { \
        if (fabsf((float)(a) - (float)(b)) > (float)(epsilon)) { \
            char _err_buf[128]; \
            snprintf(_err_buf, sizeof(_err_buf), "Expected float %.4f == %.4f (+- %.4f)", (double)(a), (double)(b), (double)(epsilon)); \
            TEST_FAIL(_err_buf); \
        } \
    } while(0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            char _err_buf[128]; \
            snprintf(_err_buf, sizeof(_err_buf), "Expected string '%s' == '%s'", (a), (b)); \
            TEST_FAIL(_err_buf); \
        } \
    } while(0)

// Callback-safe variants — for use inside bool-returning callbacks.
// These record failure and return false instead of plain void return.
#define CB_ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            printf("FAIL (callback assert: '" #cond "' is false at %s:%d)\n", __FILE__, __LINE__); \
            g_tests_failed++; \
            return false; \
        } \
    } while(0)

#define CB_ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            printf("FAIL (callback assert: %ld != %ld at %s:%d)\n", (long)(a), (long)(b), __FILE__, __LINE__); \
            g_tests_failed++; \
            return false; \
        } \
    } while(0)

// Standalone inline assertion (used in Phase 2A suite)
#define TEST_ASSERT(cond, msg) \
    do { \
        g_tests_run++; \
        if (cond) { \
            printf("  [PASS] %s\n", msg); \
            g_tests_passed++; \
        } else { \
            printf("  [FAIL] %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            g_tests_failed++; \
        } \
        fflush(stdout); \
    } while(0)
