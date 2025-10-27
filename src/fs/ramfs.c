#include <agave/kmem.h>
#include <agave/fs/ramfs.h>
#include <agave/fs.h>

ramfs_t* ramfs_create() {
    ramfs_t *fs = (ramfs_t *)kmalloc(sizeof(ramfs_t));

    if (fs)
        kmemset(fs, 0, sizeof(ramfs_t));

    return fs;
}

void ramfs_destroy(ramfs_t *fs) {
    if (!fs) return;

    ramfs_file_t *current = fs->head;
    while (current) {
        ramfs_file_t *next = current->next;
        kfree(current->data);
        kfree(current);
        current = next;
    }
    kfree(fs);
}

// Nothing to do for ramfs mount
void ramfs_mount(ramfs_t *fs) {}

fs_backend_t ramfs_backend = {
    .create = (void*(*)(void))ramfs_create,
    .destroy = (void(*)(void*))ramfs_destroy,
    .mount = (void(*)(void*))ramfs_mount
};