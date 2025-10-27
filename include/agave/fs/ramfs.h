#ifndef AGAVE_FS_RAMFS_H
#define AGAVE_FS_RAMFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct fs_backend fs_backend_t;

/*
structure of `metadata` field in `ramfs_file_t`:
- bits 0-3: file permissions (rwx for user, group, others)
- bits 4-7: file type (regular, directory, symlink, etc.)
*/

#define RAMFS_FILE_PERM_MASK      0x0F
#define RAMFS_FILE_TYPE_MASK      0xF0
#define RAMFS_FILE_TYPE_SHIFT     4

#define RAMFS_FILE_TYPE_REGULAR  0x00
#define RAMFS_FILE_TYPE_DIRECTORY 0x10
#define RAMFS_FILE_TYPE_SYMLINK   0x20

#define RAMFS_FILE_PERM_READ     0x01
#define RAMFS_FILE_PERM_WRITE    0x02
#define RAMFS_FILE_PERM_EXECUTE  0x04

typedef struct ramfs ramfs_t;
typedef struct ramfs_file ramfs_file_t;

struct ramfs_file {
    const char *name;
    void *data;
    ramfs_file_t *next;
    size_t size;
    uint8_t metadata;
};

struct ramfs {
    ramfs_file_t *head;
    size_t total_size;
};

static inline uint8_t ramfs_get_file_type(ramfs_file_t *file) {
    return (file->metadata & RAMFS_FILE_TYPE_MASK);
}

static inline uint8_t ramfs_get_file_permissions(ramfs_file_t *file) {
    return (file->metadata & RAMFS_FILE_PERM_MASK);
}

static inline void ramfs_set_file_type(ramfs_file_t *file, uint8_t type) {
    file->metadata = (file->metadata & ~RAMFS_FILE_TYPE_MASK) | (type & RAMFS_FILE_TYPE_MASK);
}

static inline void ramfs_set_file_permissions(ramfs_file_t *file, uint8_t perms) {
    file->metadata = (file->metadata & ~RAMFS_FILE_PERM_MASK) | (perms & RAMFS_FILE_PERM_MASK);
}

static inline bool ramfs_is_directory(ramfs_file_t *file) {
    return ramfs_get_file_type(file) == RAMFS_FILE_TYPE_DIRECTORY;
}

static inline bool ramfs_is_regular_file(ramfs_file_t *file) {
    return ramfs_get_file_type(file) == RAMFS_FILE_TYPE_REGULAR;
}

static inline bool ramfs_is_symlink(ramfs_file_t *file) {
    return ramfs_get_file_type(file) == RAMFS_FILE_TYPE_SYMLINK;
}

static inline bool ramfs_has_permission(ramfs_file_t *file, uint8_t perm) {
    return (ramfs_get_file_permissions(file) & perm) != 0;
}

ramfs_t* ramfs_create();
void ramfs_destroy(ramfs_t *fs);

extern fs_backend_t ramfs_backend;

#endif // AGAVE_FS_RAMFS_H