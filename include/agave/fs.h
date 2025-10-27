#ifndef AGAVE_FS_H
#define AGAVE_FS_H

#include "stdbool.h"
#include <agave/fs/ramfs.h>
#include <stddef.h>

#define MAX_FS 4

#define FS_FLAG_READONLY 0x01
#define FS_FLAG_MOUNTED  0x02
#define FS_FLAG_PRIMARY  0x04

typedef struct fs_backend {
    void* (*create)(void);
    void  (*destroy)(void *fs);
    void  (*mount)(void *fs);
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