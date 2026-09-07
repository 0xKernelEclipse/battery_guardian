#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Standard IEEE 802.3 CRC32 calculation
static inline uint32_t crc32_calc_buffer(uint32_t init, const void* buffer, size_t length) {
    const uint8_t* p = (const uint8_t*)buffer;
    uint32_t crc = ~init;
    for(size_t i = 0; i < length; i++) {
        crc ^= p[i];
        for(int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(int)(crc & 1)));
        }
    }
    return ~crc;
}

#ifdef __cplusplus
}
#endif
