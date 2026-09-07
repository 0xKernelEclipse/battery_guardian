#include "journal.h"
#include <furi.h>
#include <toolbox/crc32_calc.h>
#include <string.h>

static Storage* journal_storage = NULL;
static File* journal_file = NULL;
static const char* journal_filepath = NULL;

// Ring buffer for samples
static BatteryTelemetry sample_buffer[TELEMETRY_BUFFER_SIZE];
static uint32_t buffer_head = 0; // write index
static uint32_t buffer_tail = 0; // read index
static uint32_t buffer_count = 0;
static uint32_t dropped_sample_count = 0;

static uint32_t record_sequence = 0;
static uint32_t current_device_session_id = 0;
static FuriMutex* journal_mutex = NULL;
static bool quota_warning_logged = false;

bool journal_init(Storage* storage, const char* filepath) {
    if(!storage || !filepath) return false;
    
    if(!journal_mutex) {
        journal_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    }
    
    furi_mutex_acquire(journal_mutex, FuriWaitForever);
    
    journal_storage = storage;
    journal_filepath = filepath;
    journal_file = storage_file_alloc(storage);
    
    buffer_head = 0;
    buffer_tail = 0;
    buffer_count = 0;
    dropped_sample_count = 0;
    quota_warning_logged = false;
    current_device_session_id = furi_get_tick();
    
    // First, perform recovery if file exists
    uint32_t valid_records = 0;
    uint64_t valid_bytes = 0;
    JournalStatus status = journal_recover_ex(storage, filepath, &valid_records, &valid_bytes);
    if(status == JournalStatusErrorVersionMismatch) {
        FURI_LOG_E("BatGuard", "Refusing to open journal with unsupported format version");
        storage_file_free(journal_file);
        journal_file = NULL;
        furi_mutex_release(journal_mutex);
        return false;
    }
    record_sequence = valid_records;

    // Open in APPEND mode
    if (!storage_file_open(journal_file, filepath, FSAM_WRITE | FSAM_READ, FSOM_OPEN_APPEND)) {
        FURI_LOG_E("BatGuard", "Failed to open journal file");
        storage_file_free(journal_file);
        journal_file = NULL;
        furi_mutex_release(journal_mutex);
        return false;
    }
    
    // Write global header if file is brand new
    uint64_t file_size = storage_file_size(journal_file);
    if (file_size == 0) {
        JournalGlobalHeader global_header;
        global_header.magic = JOURNAL_MAGIC;
        global_header.format_version = JOURNAL_FORMAT_VERSION;
        global_header.device_session_id = current_device_session_id;
        storage_file_write(journal_file, &global_header, sizeof(JournalGlobalHeader));
        storage_file_sync(journal_file);
    }
    
    furi_mutex_release(journal_mutex);
    return true;
}

static bool journal_flush_locked(void);

void journal_free(void) {
    if(journal_mutex) {
        furi_mutex_acquire(journal_mutex, FuriWaitForever);
    }
    
    if (journal_file) {
        journal_flush_locked();
        storage_file_close(journal_file);
        storage_file_free(journal_file);
        journal_file = NULL;
    }
    journal_storage = NULL;
    journal_filepath = NULL;
    
    if(journal_mutex) {
        furi_mutex_release(journal_mutex);
        furi_mutex_free(journal_mutex);
        journal_mutex = NULL;
    }
}

bool journal_write_record(JournalRecordType type, uint8_t version, uint64_t timestamp, const void* payload, uint16_t length) {
    if (!journal_file || !payload || length == 0 || length > 1024) return false;
    if (type < RecordTypeSample || type > RecordTypeCheckpoint) return false;
    
    furi_mutex_acquire(journal_mutex, FuriWaitForever);
    
    // Check file size quota
    uint64_t current_size = storage_file_size(journal_file);
    uint32_t record_bytes = sizeof(JournalRecordHeader) + length + sizeof(uint32_t);
    if (current_size + record_bytes > JOURNAL_MAX_FILE_SIZE) {
        if(!quota_warning_logged) {
            FURI_LOG_W("BatGuard", "Journal file size limit reached (%u bytes) - dropping record", (unsigned int)JOURNAL_MAX_FILE_SIZE);
            quota_warning_logged = true;
        }
        furi_mutex_release(journal_mutex);
        return false;
    }

    JournalRecordHeader header;
    header.type = (uint8_t)type;
    header.version = version;
    header.length = length;
    header.sequence = record_sequence;
    header.timestamp = timestamp;
    
    // Write Header
    uint16_t bytes_written = storage_file_write(journal_file, &header, sizeof(JournalRecordHeader));
    if (bytes_written == sizeof(JournalRecordHeader)) {
        // Write Payload
        bytes_written = storage_file_write(journal_file, payload, length);
        if (bytes_written == length) {
            // Write CRC32 of Payload
            uint32_t crc = crc32_calc_buffer((uint32_t)0xFFFFFFFF, payload, length);
            bytes_written = storage_file_write(journal_file, &crc, sizeof(uint32_t));
            
            if (bytes_written == sizeof(uint32_t)) {
                storage_file_sync(journal_file);
                record_sequence++;
                furi_mutex_release(journal_mutex);
                return true;
            }
        }
    }
    
    furi_mutex_release(journal_mutex);
    return false;
}

static bool journal_flush_locked(void) {
    if (!journal_file || buffer_count == 0) return true;
    
    bool all_written = true;
    while(buffer_count > 0) {
        BatteryTelemetry* sample = &sample_buffer[buffer_tail];
        
        // Check file size quota
        uint64_t current_size = storage_file_size(journal_file);
        uint32_t sample_bytes = sizeof(JournalRecordHeader) + sizeof(BatteryTelemetry) + sizeof(uint32_t);
        if (current_size + sample_bytes > JOURNAL_MAX_FILE_SIZE) {
            if(!quota_warning_logged) {
                FURI_LOG_W("BatGuard", "Journal file size limit reached (%u bytes) during flush - dropping samples", (unsigned int)JOURNAL_MAX_FILE_SIZE);
                quota_warning_logged = true;
            }
            // Evict remaining buffered samples to prevent infinite buffer growth
            buffer_count = 0;
            buffer_tail = buffer_head;
            all_written = false;
            break;
        }

        // Write single record directly inside the locked section
        JournalRecordHeader header;
        header.type = (uint8_t)RecordTypeSample;
        header.version = SAMPLE_PAYLOAD_VERSION;
        header.length = sizeof(BatteryTelemetry);
        header.sequence = record_sequence;
        header.timestamp = sample->timestamp_ms;
        
        uint16_t hw = storage_file_write(journal_file, &header, sizeof(JournalRecordHeader));
        uint16_t pw = storage_file_write(journal_file, sample, sizeof(BatteryTelemetry));
        uint32_t crc = crc32_calc_buffer((uint32_t)0xFFFFFFFF, sample, sizeof(BatteryTelemetry));
        uint16_t cw = storage_file_write(journal_file, &crc, sizeof(uint32_t));
        
        if (hw == sizeof(JournalRecordHeader) && pw == sizeof(BatteryTelemetry) && cw == sizeof(uint32_t)) {
            record_sequence++;
            buffer_tail = (buffer_tail + 1) % TELEMETRY_BUFFER_SIZE;
            buffer_count--;
        } else {
            all_written = false;
            break;
        }
    }
    
    if(all_written) {
        storage_file_sync(journal_file);
    }
    
    return all_written;
}

bool journal_flush(void) {
    if (!journal_file || buffer_count == 0) return true;
    if (!journal_mutex) return false;
    
    furi_mutex_acquire(journal_mutex, FuriWaitForever);
    bool result = journal_flush_locked();
    furi_mutex_release(journal_mutex);
    return result;
}

bool journal_enqueue_sample(const BatteryTelemetry* sample) {
    if (!sample) return false;
    
    if(!journal_mutex) return false;
    furi_mutex_acquire(journal_mutex, FuriWaitForever);
    
    if (buffer_count >= TELEMETRY_BUFFER_SIZE) {
        // Evict oldest sample to keep ring buffer strictly bounded
        buffer_tail = (buffer_tail + 1) % TELEMETRY_BUFFER_SIZE;
        buffer_count--;
        dropped_sample_count++;
    }
    
    sample_buffer[buffer_head] = *sample;
    buffer_head = (buffer_head + 1) % TELEMETRY_BUFFER_SIZE;
    buffer_count++;
    
    bool should_flush = (buffer_count >= (TELEMETRY_BUFFER_SIZE / 2));
    furi_mutex_release(journal_mutex);
    
    if (should_flush && journal_file) {
        journal_flush();
    }
    
    return true;
}

uint32_t journal_get_buffered_count(void) {
    return buffer_count;
}

uint32_t journal_get_dropped_count(void) {
    return dropped_sample_count;
}

JournalStatus journal_recover_ex(Storage* storage, const char* filepath, uint32_t* out_valid_records, uint64_t* out_valid_bytes) {
    if(!storage || !filepath) return JournalStatusErrorIo;
    
    File* file = storage_file_alloc(storage);
    if(!file) return JournalStatusErrorIo;
    
    if(!storage_file_open(file, filepath, FSAM_READ | FSAM_WRITE, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        if(out_valid_records) *out_valid_records = 0;
        if(out_valid_bytes) *out_valid_bytes = 0;
        return JournalStatusOk; // Clean start if file doesn't exist
    }
    
    uint64_t file_size = storage_file_size(file);
    if(file_size < sizeof(JournalGlobalHeader)) {
        // File too small to have a global header. If size > 0, truncate to 0.
        if(file_size > 0) {
            storage_file_close(file);
            storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS);
        }
        storage_file_close(file);
        storage_file_free(file);
        if(out_valid_records) *out_valid_records = 0;
        if(out_valid_bytes) *out_valid_bytes = 0;
        return (file_size > 0) ? JournalStatusErrorCorrupt : JournalStatusOk;
    }
    
    // Read and verify global header
    JournalGlobalHeader gh;
    if(storage_file_read(file, &gh, sizeof(JournalGlobalHeader)) != sizeof(JournalGlobalHeader)) {
        storage_file_close(file);
        storage_file_free(file);
        return JournalStatusErrorCorrupt;
    }
    
    if(gh.magic != JOURNAL_MAGIC) {
        storage_file_close(file);
        storage_file_free(file);
        return JournalStatusErrorCorrupt;
    }

    if(gh.format_version > JOURNAL_FORMAT_VERSION) {
        // Incompatible future format version: preserve data intact, do not truncate!
        FURI_LOG_W("BatGuard", "Journal format version %u unsupported (max %u)", 
                   gh.format_version, JOURNAL_FORMAT_VERSION);
        storage_file_close(file);
        storage_file_free(file);
        return JournalStatusErrorVersionMismatch;
    }

    if(gh.format_version == 0) {
        storage_file_close(file);
        storage_file_free(file);
        return JournalStatusErrorCorrupt;
    }
    
    uint64_t valid_offset = sizeof(JournalGlobalHeader);
    uint32_t count = 0;
    uint8_t payload_buf[1024];
    
    while(true) {
        storage_file_seek(file, (uint32_t)valid_offset, true);
        
        JournalRecordHeader rh;
        uint16_t r = storage_file_read(file, &rh, sizeof(JournalRecordHeader));
        if(r < sizeof(JournalRecordHeader)) {
            break; // Truncated or EOF
        }
        
        if(rh.type < RecordTypeSample || rh.type > RecordTypeCheckpoint) {
            break; // Unknown record type
        }
        
        if(rh.length == 0 || rh.length > sizeof(payload_buf)) {
            break; // Corrupted or oversized length
        }
        
        r = storage_file_read(file, payload_buf, rh.length);
        if(r < rh.length) {
            break; // Truncated payload
        }
        
        uint32_t stored_crc = 0;
        r = storage_file_read(file, &stored_crc, sizeof(uint32_t));
        if(r < sizeof(uint32_t)) {
            break; // Truncated CRC
        }
        
        uint32_t computed_crc = crc32_calc_buffer((uint32_t)0xFFFFFFFF, payload_buf, rh.length);
        if(computed_crc != stored_crc) {
            break; // Corrupted payload or CRC mismatch
        }
        
        // Record is 100% valid!
        valid_offset += sizeof(JournalRecordHeader) + rh.length + sizeof(uint32_t);
        count++;
    }
    
    // If there is trailing corrupted/incomplete data, truncate it safely without unbounded malloc
    if(valid_offset < file_size) {
        storage_file_seek(file, (uint32_t)valid_offset, true);
        if(!storage_file_truncate(file)) {
            // Streaming copy fallback in 512-byte chunks using a temporary file
            char tmp_path[128];
            snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", filepath);
            File* tmp_file = storage_file_alloc(storage);
            if(tmp_file && storage_file_open(tmp_file, tmp_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
                storage_file_seek(file, 0, true);
                uint8_t chunk[512];
                uint64_t copied = 0;
                while(copied < valid_offset) {
                    uint16_t to_read = (uint16_t)(((valid_offset - copied) < sizeof(chunk)) ? (valid_offset - copied) : sizeof(chunk));
                    uint16_t r = storage_file_read(file, chunk, to_read);
                    if(r == 0) break;
                    storage_file_write(tmp_file, chunk, r);
                    copied += r;
                }
                storage_file_sync(tmp_file);
                storage_file_close(tmp_file);
                
                // Rewrite target file with verified contents
                if(copied == valid_offset && storage_file_open(tmp_file, tmp_path, FSAM_READ, FSOM_OPEN_EXISTING)) {
                    storage_file_close(file);
                    if(storage_file_open(file, filepath, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
                        uint64_t written = 0;
                        while(written < valid_offset) {
                            uint16_t to_read = (uint16_t)(((valid_offset - written) < sizeof(chunk)) ? (valid_offset - written) : sizeof(chunk));
                            uint16_t r = storage_file_read(tmp_file, chunk, to_read);
                            if(r == 0) break;
                            storage_file_write(file, chunk, r);
                            written += r;
                        }
                        storage_file_sync(file);
                    }
                    storage_file_close(tmp_file);
                }
            }
            if(tmp_file) storage_file_free(tmp_file);
        } else {
            storage_file_sync(file);
        }
    }
    
    storage_file_close(file);
    storage_file_free(file);
    
    if(out_valid_records) *out_valid_records = count;
    if(out_valid_bytes) *out_valid_bytes = valid_offset;
    return JournalStatusOk;
}

bool journal_recover(Storage* storage, const char* filepath, uint32_t* out_valid_records, uint64_t* out_valid_bytes) {
    return (journal_recover_ex(storage, filepath, out_valid_records, out_valid_bytes) == JournalStatusOk);
}

bool journal_iterate_records(Storage* storage, const char* filepath, JournalRecordCallback callback, void* context) {
    if(!storage || !filepath || !callback) return false;
    
    File* file = storage_file_alloc(storage);
    if(!file) return false;
    
    if(!storage_file_open(file, filepath, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        return false;
    }
    
    JournalGlobalHeader gh;
    if(storage_file_read(file, &gh, sizeof(JournalGlobalHeader)) != sizeof(JournalGlobalHeader)) {
        storage_file_close(file);
        storage_file_free(file);
        return false;
    }
    
    if(gh.magic != JOURNAL_MAGIC || gh.format_version != JOURNAL_FORMAT_VERSION) {
        storage_file_close(file);
        storage_file_free(file);
        return false;
    }
    
    uint8_t payload_buf[1024];
    while(true) {
        JournalRecordHeader rh;
        uint16_t r = storage_file_read(file, &rh, sizeof(JournalRecordHeader));
        if(r < sizeof(JournalRecordHeader)) break;
        
        if(rh.type < RecordTypeSample || rh.type > RecordTypeCheckpoint) break;
        if(rh.length == 0 || rh.length > sizeof(payload_buf)) break;
        
        r = storage_file_read(file, payload_buf, rh.length);
        if(r < rh.length) break;
        
        uint32_t stored_crc = 0;
        r = storage_file_read(file, &stored_crc, sizeof(uint32_t));
        if(r < sizeof(uint32_t)) break;
        
        uint32_t computed_crc = crc32_calc_buffer((uint32_t)0xFFFFFFFF, payload_buf, rh.length);
        if(computed_crc != stored_crc) break;
        
        if(!callback(&rh, payload_buf, context)) {
            break;
        }
    }
    
    storage_file_close(file);
    storage_file_free(file);
    return true;
}
