#include <agave/fs.h>
#include <agave/kmem.h>
#include <agave/kcore.h>
#include <string.h>

static fs_t *mounted_fs[MAX_FS];
static size_t fs_count = 0;

void fs_initialize(const char *name, fs_backend_t *backend, uint8_t flags) {
    if (fs_count >= MAX_FS) {
        kpanic("too many filesystems mounted");
    }

    fs_t *fs = (fs_t *)kmalloc(sizeof(fs_t));
    if (!fs) {
        kpanic("failed to allocate memory for filesystem '%s'", name);
    }

    fs->name = name;
    fs->backend = backend;
    fs->flags = flags;
    fs->backend_data = backend->create();

    if (!fs->backend_data) {
        kfree(fs);
        kpanic("failed to initialize filesystem backend for '%s'", name);
    }

    mounted_fs[fs_count++] = fs;

    if (flags & FS_FLAG_PRIMARY) {
        kcore_get_information()->fs = fs;
    }
}

void fs_mount_all(void) {
    if (fs_count == 0) {
        kpanic("no filesystems initialized to mount");
    }

    for (size_t i = 0; i < fs_count; i++) {
        fs_t *fs = mounted_fs[i];
        if (fs->backend && fs->backend->mount) {
            fs->backend->mount(fs->backend_data);
            fs->flags |= FS_FLAG_MOUNTED;
        } else {
            kpanic("filesystem backend for '%s' does not support mounting", fs->name);
        }
    }
}

fs_t** fs_get_mounted(size_t *count) {
    if (count) *count = fs_count;
    return mounted_fs;
}

fs_t *fs_get_primary(void) {
    for (size_t i = 0; i < fs_count; i++) {
        if (mounted_fs[i]->flags & FS_FLAG_PRIMARY)
            return mounted_fs[i];
    }
    return NULL;
}

void fs_switch_primary(const char *name) {
    for (size_t i = 0; i < fs_count; i++) {
        if (strcmp(mounted_fs[i]->name, name) == 0) {
            for (size_t j = 0; j < fs_count; j++) {
                mounted_fs[j]->flags &= ~FS_FLAG_PRIMARY;
            }
            mounted_fs[i]->flags |= FS_FLAG_PRIMARY;
            kcore_get_information()->fs = mounted_fs[i];
            return;
        }
    }
    kpanic("filesystem '%s' not found to switch primary", name);
}

void fs_shutdown_all(void) {
    for (size_t i = 0; i < fs_count; i++) {
        fs_t *fs = mounted_fs[i];
        if (fs->backend && fs->backend->destroy) {
            fs->backend->destroy(fs->backend_data);
        }
        kfree(fs);
    }
    fs_count = 0;
}