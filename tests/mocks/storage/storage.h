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

typedef enum {
    FSE_OK = 0,
    FSE_NOT_READY,
    FSE_EXIST,
    FSE_NOT_EXIST,
    FSE_INVALID_PARAMETER,
    FSE_DENIED,
    FSE_INVALID_NAME,
    FSE_INTERNAL,
} FS_Error;

#define FSAM_READ  (1 << 0)
#define FSAM_WRITE (1 << 1)

#define FSOM_OPEN_EXISTING 1
#define FSOM_OPEN_ALWAYS   2
#define FSOM_OPEN_APPEND   3
#define FSOM_CREATE_NEW    4
#define FSOM_CREATE_ALWAYS 5

#define EXT_PATH(p) "/ext/" p
#define RECORD_STORAGE "storage"

typedef struct Storage Storage;
typedef struct File File;

// Mock virtual in-memory file system
#define MOCK_FS_MAX_FILES 8
#define MOCK_FS_MAX_FILE_SIZE (2 * 1024 * 1024)

typedef struct {
    char name[64];
    uint8_t* data;
    size_t size;
    size_t capacity;
    bool in_use;
} MockVirtualFile;

struct Storage {
    bool sd_available;
    bool simulate_disk_full;
    size_t max_allowed_bytes;
    MockVirtualFile files[MOCK_FS_MAX_FILES];
};

struct File {
    Storage* storage;
    int file_index;
    size_t cursor;
    uint32_t access_mode;
    uint32_t open_mode;
    bool is_open;
};

extern Storage g_mock_storage;

static inline void mock_storage_reset(void) {
    g_mock_storage.sd_available = true;
    g_mock_storage.simulate_disk_full = false;
    g_mock_storage.max_allowed_bytes = MOCK_FS_MAX_FILE_SIZE;
    for(int i = 0; i < MOCK_FS_MAX_FILES; i++) {
        if(g_mock_storage.files[i].data) {
            free(g_mock_storage.files[i].data);
            g_mock_storage.files[i].data = NULL;
        }
        g_mock_storage.files[i].size = 0;
        g_mock_storage.files[i].capacity = 0;
        g_mock_storage.files[i].in_use = false;
        g_mock_storage.files[i].name[0] = '\0';
    }
}

static inline FS_Error storage_sd_status(Storage* storage) {
    if(!storage || !storage->sd_available) return FSE_NOT_READY;
    return FSE_OK;
}

static inline Storage* furi_record_open(const char* name) {
    if(strcmp(name, RECORD_STORAGE) == 0) {
        return &g_mock_storage;
    }
    return NULL;
}

static inline void furi_record_close(const char* name) {
    (void)name;
}

static inline File* storage_file_alloc(Storage* storage) {
    File* f = (File*)calloc(1, sizeof(File));
    if(f) {
        f->storage = storage ? storage : &g_mock_storage;
        f->file_index = -1;
    }
    return f;
}

static inline void storage_file_free(File* file) {
    if(file) free(file);
}

static inline bool storage_file_open(File* file, const char* path, uint32_t access_mode, uint32_t open_mode) {
    if(!file || !file->storage || !file->storage->sd_available) return false;
    
    // Find or allocate file slot
    int slot = -1;
    for(int i = 0; i < MOCK_FS_MAX_FILES; i++) {
        if(file->storage->files[i].in_use && strcmp(file->storage->files[i].name, path) == 0) {
            slot = i;
            break;
        }
    }
    
    if(slot == -1) {
        // Allocate new slot
        for(int i = 0; i < MOCK_FS_MAX_FILES; i++) {
            if(!file->storage->files[i].in_use) {
                slot = i;
                file->storage->files[i].in_use = true;
                strncpy(file->storage->files[i].name, path, sizeof(file->storage->files[i].name) - 1);
                file->storage->files[i].capacity = 4096;
                file->storage->files[i].data = (uint8_t*)malloc(file->storage->files[i].capacity);
                file->storage->files[i].size = 0;
                break;
            }
        }
    }
    
    if(slot == -1) return false;
    
    file->file_index = slot;
    file->access_mode = access_mode;
    file->open_mode = open_mode;
    file->is_open = true;
    
    if(open_mode == FSOM_CREATE_ALWAYS) {
        file->storage->files[slot].size = 0;
        file->cursor = 0;
    } else if(open_mode == FSOM_OPEN_APPEND) {
        file->cursor = file->storage->files[slot].size;
    } else {
        file->cursor = 0;
    }
    
    return true;
}

static inline bool storage_file_close(File* file) {
    if(!file || !file->is_open) return false;
    file->is_open = false;
    return true;
}

static inline uint64_t storage_file_size(File* file) {
    if(!file || !file->is_open || file->file_index < 0) return 0;
    return file->storage->files[file->file_index].size;
}

static inline bool storage_file_sync(File* file) {
    if(!file || !file->is_open) return false;
    return true;
}

static inline bool storage_file_truncate(File* file) {
    if(!file || !file->is_open || file->file_index < 0) return false;
    MockVirtualFile* vf = &file->storage->files[file->file_index];
    if(file->cursor <= vf->size) {
        vf->size = file->cursor;
    }
    return true;
}

static inline bool storage_common_remove(Storage* storage, const char* path) {
    if(!storage || !path) return false;
    for(int i = 0; i < MOCK_FS_MAX_FILES; i++) {
        if(storage->files[i].in_use && strcmp(storage->files[i].name, path) == 0) {
            storage->files[i].in_use = false;
            storage->files[i].size = 0;
            storage->files[i].name[0] = '\0';
            return true;
        }
    }
    return false;
}

static inline uint16_t storage_file_write(File* file, const void* buffer, uint16_t bytes_to_write) {
    if(!file || !file->is_open || file->file_index < 0 || !buffer) return 0;
    if(file->storage->simulate_disk_full) return 0;
    
    MockVirtualFile* vf = &file->storage->files[file->file_index];
    size_t new_pos = file->cursor + bytes_to_write;
    
    if(new_pos > file->storage->max_allowed_bytes) {
        return 0;
    }
    
    if(new_pos > vf->capacity) {
        size_t new_cap = vf->capacity * 2;
        if(new_cap < new_pos) new_cap = new_pos + 4096;
        uint8_t* new_data = (uint8_t*)realloc(vf->data, new_cap);
        if(!new_data) return 0;
        vf->data = new_data;
        vf->capacity = new_cap;
    }
    
    memcpy(vf->data + file->cursor, buffer, bytes_to_write);
    file->cursor = new_pos;
    if(file->cursor > vf->size) {
        vf->size = file->cursor;
    }
    
    return bytes_to_write;
}

static inline uint16_t storage_file_read(File* file, void* buffer, uint16_t bytes_to_read) {
    if(!file || !file->is_open || file->file_index < 0 || !buffer) return 0;
    
    MockVirtualFile* vf = &file->storage->files[file->file_index];
    if(file->cursor >= vf->size) return 0;
    
    size_t available = vf->size - file->cursor;
    size_t to_read = (bytes_to_read > available) ? available : bytes_to_read;
    
    memcpy(buffer, vf->data + file->cursor, to_read);
    file->cursor += to_read;
    return (uint16_t)to_read;
}

static inline bool storage_file_seek(File* file, uint32_t offset, bool from_start) {
    if(!file || !file->is_open || file->file_index < 0) return false;
    MockVirtualFile* vf = &file->storage->files[file->file_index];
    
    if(from_start) {
        if(offset > vf->size) return false;
        file->cursor = offset;
    } else {
        if(file->cursor + offset > vf->size) return false;
        file->cursor += offset;
    }
    return true;
}

#ifdef __cplusplus
}
#endif
