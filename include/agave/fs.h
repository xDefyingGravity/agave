#ifndef AGAVE_FS_H
#define AGAVE_FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <agave/fs/ramfs.h>

#define MAX_FS 4

#define FS_FLAG_READONLY 0x01
#define FS_FLAG_MOUNTED  0x02
#define FS_FLAG_PRIMARY  0x04

typedef enum {
    FS_STATUS_OK = 0,
    FS_STATUS_ERROR_NO_ENTRY,
    FS_STATUS_ERROR_PERMISSION_DENIED,
    FS_STATUS_ERROR_ALREADY_EXISTS,
    FS_STATUS_ERROR_INVALID_TYPE,
    FS_STATUS_ERROR_NOT_DIRECTORY,
    FS_STATUS_ERROR_DIRECTORY_NOT_EMPTY,
    FS_STATUS_ERROR_READ_ONLY,
    FS_STATUS_ERROR_NO_SPACE,
    FS_STATUS_ERROR_INVALID_ARGUMENT,
    FS_STATUS_ERROR_UNKNOWN
} fs_status_t;

#define FS_PERM_READ    RAMFS_FILE_PERM_READ
#define FS_PERM_WRITE   RAMFS_FILE_PERM_WRITE
#define FS_PERM_EXECUTE RAMFS_FILE_PERM_EXECUTE

typedef struct fs_backend {
    void* (*create)(void);
    void  (*destroy)(void *fs);
    void  (*mount)(void *fs);

    void  (*unmount)(void *fs);

    // file operations
    fs_status_t (*add_file)(void *fs, const char *path, const void *data, size_t size, uint8_t metadata);
    fs_status_t (*read_file)(void *fs, const char *path, const void **out_data, size_t *size);
    fs_status_t (*remove_file)(void *fs, const char *path);
    fs_status_t (*write_file)(void *fs, const char *path, const void *data, size_t size);
    fs_status_t (*file_exists)(void *fs, const char *path, bool *out_exists);
    fs_status_t (*file_size)(void *fs, const char *path, size_t *out_size);

    // folder operations
    fs_status_t (*make_directory)(void *fs, const char *path);
    fs_status_t (*remove_directory)(void *fs, const char *path);
    fs_status_t (*directory_exists)(void *fs, const char *path, bool *out_exists);
    fs_status_t (*directory_size)(void *fs, const char *path, size_t *out_size);
    fs_status_t (*list_directory)(void *fs, const char *path, char ***out_list, size_t max_entries, size_t *out_count);

    // metadata operations
    fs_status_t (*get_file_metadata)(void *fs, const char *path, uint8_t *out_metadata);

    // permission operations
    fs_status_t (*set_file_permissions)(void *fs, const char *path, uint8_t permissions);
    fs_status_t (*get_file_permissions)(void *fs, const char *path, uint8_t *out_permissions);
} fs_backend_t;

typedef struct fs {
    const char *name;
    fs_backend_t *backend;
    void *backend_data;
    uint8_t flags;
} fs_t;

#define RAMFS (&ramfs_backend)

void fs_initialize(const char *name, fs_backend_t *backend, uint8_t flags);
void fs_mount_all(void);

fs_t** fs_get_mounted(size_t *count);

void fs_switch_primary(const char *name);

void fs_shutdown_all(void);

fs_status_t fs_add_file(fs_t *fs, const char *path, const void *data, size_t size, uint8_t metadata);
fs_status_t fs_read_file(fs_t *fs, const char *path, const void **out_data, size_t *out_size);
fs_status_t fs_remove_file(fs_t *fs, const char *path);
fs_status_t fs_write_file(fs_t *fs, const char *path, const void *data, size_t size);
fs_status_t fs_file_exists(fs_t *fs, const char *path, bool *out_exists);
fs_status_t fs_file_size(fs_t *fs, const char *path, size_t *out_size);

fs_status_t fs_make_directory(fs_t *fs, const char *path);
fs_status_t fs_remove_directory(fs_t *fs, const char *path);
fs_status_t fs_directory_exists(fs_t *fs, const char *path, bool *out_exists);
fs_status_t fs_directory_size(fs_t *fs, const char *path, size_t *out_size);
fs_status_t fs_list_directory(fs_t *fs, const char *path, char ***out_list, size_t max_entries, size_t *out_count);

fs_status_t fs_get_file_metadata(fs_t *fs, const char *path, uint8_t *out_metadata);
fs_status_t fs_set_file_permissions(fs_t *fs, const char *path, uint8_t permissions);
fs_status_t fs_get_file_permissions(fs_t *fs, const char *path, uint8_t *out_permissions);

const char *fs_status_to_string(fs_status_t status);

static inline bool fs_is_mounted(fs_t *fs) {
    return (fs->flags & FS_FLAG_MOUNTED) != 0;
}

static inline bool fs_is_readonly(fs_t *fs) {
    return (fs->flags & FS_FLAG_READONLY) != 0;
}

static inline bool fs_is_primary(fs_t *fs) {
    return (fs->flags & FS_FLAG_PRIMARY) != 0;
}

#endif // AGAVE_FS_H