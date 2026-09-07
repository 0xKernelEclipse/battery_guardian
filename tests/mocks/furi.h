#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UNUSED(x) (void)(x)

// Logging macros for tests
#define FURI_LOG_I(tag, fmt, ...) printf("[INFO][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define FURI_LOG_W(tag, fmt, ...) printf("[WARN][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define FURI_LOG_E(tag, fmt, ...) printf("[ERROR][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define FURI_LOG_D(tag, fmt, ...) printf("[DEBUG][%s] " fmt "\n", tag, ##__VA_ARGS__)

// Mock Tick Timer
extern uint32_t g_mock_tick_ms;
static inline uint32_t furi_get_tick(void) {
    return g_mock_tick_ms;
}

static inline void furi_delay_ms(uint32_t ms) {
    g_mock_tick_ms += ms;
}

// Mock Mutex
typedef struct FuriMutex {
    int locked;
} FuriMutex;

typedef enum {
    FuriMutexTypeNormal = 0
} FuriMutexType;

#define FuriWaitForever 0xFFFFFFFF

static inline FuriMutex* furi_mutex_alloc(FuriMutexType type) {
    UNUSED(type);
    FuriMutex* m = (FuriMutex*)malloc(sizeof(FuriMutex));
    if(m) m->locked = 0;
    return m;
}

static inline void furi_mutex_free(FuriMutex* mutex) {
    if(mutex) free(mutex);
}

static inline bool furi_mutex_acquire(FuriMutex* mutex, uint32_t timeout) {
    UNUSED(timeout);
    if(mutex) {
        mutex->locked++;
        return true;
    }
    return false;
}

static inline bool furi_mutex_release(FuriMutex* mutex) {
    if(mutex && mutex->locked > 0) {
        mutex->locked--;
        return true;
    }
    return false;
}

#ifdef __cplusplus
}
#endif
