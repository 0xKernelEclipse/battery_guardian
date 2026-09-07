#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <storage/storage.h>
#include "../core/telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JOURNAL_MAGIC 0x42415447 // "BATG"
#define JOURNAL_FORMAT_VERSION 1
#define JOURNAL_MAX_FILE_SIZE (512 * 1024) // 512 KB quota limit
#define SAMPLE_PAYLOAD_VERSION 1
#define SESSION_PAYLOAD_VERSION 1
#define EVENT_PAYLOAD_VERSION 1
#define ESTIMATE_PAYLOAD_VERSION 1

// Journal status and error codes
typedef enum {
    JournalStatusOk = 0,
    JournalStatusErrorIo,
    JournalStatusErrorVersionMismatch,
    JournalStatusErrorCorrupt,
    JournalStatusErrorStorageFull,
} JournalStatus;

// Record types
typedef enum {
    RecordTypeSample = 1,
    RecordTypeSession = 2,
    RecordTypeEvent = 3,
    RecordTypeEstimate = 4,
    RecordTypeCheckpoint = 5,
} JournalRecordType;

// Journal Global Header (once at the beginning of file)
typedef struct __attribute__((packed)) {
    uint32_t magic;             // 0x42415447 ("BATG")
    uint8_t format_version;     // Overall format version (e.g. 1)
    uint32_t device_session_id; // Unique ID to distinguish boot/reinstalls
} JournalGlobalHeader;

// Standard Record Header (precedes every payload)
typedef struct __attribute__((packed)) {
    uint8_t type;           // JournalRecordType
    uint8_t version;        // Payload version (allows evolving structs)
    uint16_t length;        // Size of following payload
    uint32_t sequence;      // Monotonically increasing record ID
    uint64_t timestamp;     // Monotonic timestamp
} JournalRecordHeader;

// Buffer management for telemetry
#define TELEMETRY_BUFFER_SIZE 64

// Initialize the journal system
bool journal_init(Storage* storage, const char* filepath);

// Deinitialize journal
void journal_free(void);

// Enqueue a telemetry sample to RAM ring-buffer.
// Flushes to SD when full. Evicts oldest sample if storage unavailable.
bool journal_enqueue_sample(const BatteryTelemetry* sample);

// Force flush RAM buffer to SD
bool journal_flush(void);

// General write function for other record types
bool journal_write_record(JournalRecordType type, uint8_t version, uint64_t timestamp, const void* payload, uint16_t length);

// Recover a journal file: verifies header, scans valid records, discards corrupted/truncated tail
bool journal_recover(Storage* storage, const char* filepath, uint32_t* out_valid_records, uint64_t* out_valid_bytes);
JournalStatus journal_recover_ex(Storage* storage, const char* filepath, uint32_t* out_valid_records, uint64_t* out_valid_bytes);

// Callback for iterating through validated journal records
typedef bool (*JournalRecordCallback)(const JournalRecordHeader* header, const void* payload, void* context);

// Iterate verified records in a journal file
bool journal_iterate_records(Storage* storage, const char* filepath, JournalRecordCallback callback, void* context);

// Get internal buffer diagnostics
uint32_t journal_get_buffered_count(void);
uint32_t journal_get_dropped_count(void);

#ifdef __cplusplus
}
#endif
