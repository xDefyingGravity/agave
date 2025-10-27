#ifndef AGAVE_FS_RAMFS_H
#define AGAVE_FS_RAMFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct fs_backend fs_backend_t;

#define RAMFS_FILE_PERM_MASK      0x0F
#define RAMFS_FILE_TYPE_MASK      0xF0
#define RAMFS_FILE_TYPE_SHIFT     4

#define RAMFS_FILE_TYPE_REGULAR   0x00
#define RAMFS_FILE_TYPE_DIRECTORY 0x10
#define RAMFS_FILE_TYPE_SYMLINK   0x20

#define RAMFS_FILE_PERM_READ      0x01
#define RAMFS_FILE_PERM_WRITE     0x02
#define RAMFS_FILE_PERM_EXECUTE   0x04

typedef struct ramfs ramfs_t;
typedef struct ramfs_file ramfs_file_t;

#define RAMFS_HASH_BUCKETS 64

struct ramfs_file {
    char *name;
    void *data;
    size_t size;            // only for files
    uint8_t metadata;

    ramfs_file_t *parent;
    ramfs_file_t *child;
    ramfs_file_t *sibling;

    ramfs_file_t *global_next;
    ramfs_file_t *hash_next;
};

struct ramfs {
    ramfs_file_t *head;
    ramfs_file_t *root;
    ramfs_file_t *buckets[RAMFS_HASH_BUCKETS];
    size_t total_size;
};

// helpers
static inline uint8_t ramfs_get_file_type(ramfs_file_t *file) {
    return file->metadata & RAMFS_FILE_TYPE_MASK;
}

static inline void ramfs_set_file_type(ramfs_file_t *file, uint8_t type) {
    file->metadata = (file->metadata & ~RAMFS_FILE_TYPE_MASK) | (type & RAMFS_FILE_TYPE_MASK);
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
    return (file->metadata & perm) == perm;
}

ramfs_t* ramfs_create(void);
void ramfs_destroy(ramfs_t *fs);

extern fs_backend_t ramfs_backend;

#endif // AGAVE_FS_RAMFS_H