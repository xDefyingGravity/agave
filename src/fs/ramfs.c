#include <agave/fs/ramfs.h>
#include <agave/fs.h>
#include <agave/kmem.h>
#include <stdbool.h>
#include <string.h>

#define RAMFS_DEFAULT_FILE_PERMS (FS_PERM_READ | FS_PERM_WRITE)
#define RAMFS_DEFAULT_DIR_PERMS  (FS_PERM_READ | FS_PERM_WRITE | FS_PERM_EXECUTE)

static uint32_t _ramfs_hash_string(const char *path) {
    uint32_t hash = 2166136261u;
    while (*path) {
        hash ^= (uint8_t)(*path++);
        hash *= 16777619u;
    }
    return hash;
}

static size_t _ramfs_bucket_index(const char *path) {
    return _ramfs_hash_string(path) % RAMFS_HASH_BUCKETS;
}

static void _ramfs_link_global(ramfs_t *fs, ramfs_file_t *node) {
    node->global_next = fs->head;
    fs->head = node;
}

static void _ramfs_unlink_global(ramfs_t *fs, ramfs_file_t *node) {
    ramfs_file_t *prev = NULL;
    ramfs_file_t *curr = fs->head;
    while (curr) {
        if (curr == node) {
            if (prev) {
                prev->global_next = curr->global_next;
            } else {
                fs->head = curr->global_next;
            }
            node->global_next = NULL;
            return;
        }
        prev = curr;
        curr = curr->global_next;
    }
}

static void _ramfs_insert_bucket(ramfs_t *fs, ramfs_file_t *node) {
    size_t bucket = _ramfs_bucket_index(node->name);
    node->hash_next = fs->buckets[bucket];
    fs->buckets[bucket] = node;
}

static void _ramfs_remove_bucket(ramfs_t *fs, ramfs_file_t *node) {
    size_t bucket = _ramfs_bucket_index(node->name);
    ramfs_file_t *prev = NULL;
    ramfs_file_t *curr = fs->buckets[bucket];
    while (curr) {
        if (curr == node) {
            if (prev) {
                prev->hash_next = curr->hash_next;
            } else {
                fs->buckets[bucket] = curr->hash_next;
            }
            node->hash_next = NULL;
            return;
        }
        prev = curr;
        curr = curr->hash_next;
    }
}

static void _ramfs_attach_child(ramfs_file_t *parent, ramfs_file_t *child) {
    child->sibling = parent->child;
    parent->child = child;
    child->parent = parent;
}

static void _ramfs_detach_child(ramfs_file_t *parent, ramfs_file_t *child) {
    ramfs_file_t *prev = NULL;
    ramfs_file_t *curr = parent->child;
    while (curr) {
        if (curr == child) {
            if (prev) {
                prev->sibling = curr->sibling;
            } else {
                parent->child = curr->sibling;
            }
            child->sibling = NULL;
            child->parent = NULL;
            return;
        }
        prev = curr;
        curr = curr->sibling;
    }
}

static const char *_ramfs_find_last_sep(const char *path) {
    const char *last = NULL;
    for (const char *p = path; *p; ++p) {
        if (*p == '/') {
            last = p;
        }
    }
    return last;
}

static const char *_ramfs_basename(const char *path) {
    const char *last = _ramfs_find_last_sep(path);
    return last ? last + 1 : path;
}

static char *_ramfs_normalize_path(const char *path) {
    if (!path) {
        return NULL;
    }

    size_t len = strlen(path);
    size_t buf_len = len + 2; // room for leading slash and null terminator
    char *normalized = (char *)kmalloc(buf_len);
    if (!normalized) {
        return NULL;
    }

    size_t in = 0;
    size_t out = 0;
    bool first_component = true;

    while (in < len && path[in] == '/') {
        in++;
    }

    normalized[out++] = '/';

    if (in == len) {
        normalized[out] = '\0';
        return normalized;
    }

    while (in < len) {
        size_t start = in;
        while (in < len && path[in] != '/') {
            in++;
        }

        size_t comp_len = in - start;
        if (comp_len > 0) {
            if (!first_component) {
                normalized[out++] = '/';
            }
            kmemcpy(normalized + out, path + start, comp_len);
            out += comp_len;
            first_component = false;
        }

        while (in < len && path[in] == '/') {
            in++;
        }
    }

    normalized[out] = '\0';
    return normalized;
}

static ramfs_file_t *_ramfs_lookup(ramfs_t *fs, const char *path) {
    size_t bucket = _ramfs_bucket_index(path);
    ramfs_file_t *current = fs->buckets[bucket];
    while (current) {
        if (strcmp(current->name, path) == 0) {
            return current;
        }
        current = current->hash_next;
    }
    return NULL;
}

static fs_status_t _ramfs_parent_for(ramfs_t *fs, const char *normalized_path,
                                     ramfs_file_t **out_parent) {
    const char *last_sep = _ramfs_find_last_sep(normalized_path);
    if (!last_sep) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (last_sep == normalized_path) {
        *out_parent = fs->root;
        return FS_STATUS_OK;
    }

    size_t parent_len = (size_t)(last_sep - normalized_path);
    char *parent_path = (char *)kmalloc(parent_len + 1);
    if (!parent_path) {
        return FS_STATUS_ERROR_NO_SPACE;
    }
    kmemcpy(parent_path, normalized_path, parent_len);
    parent_path[parent_len] = '\0';

    ramfs_file_t *parent = _ramfs_lookup(fs, parent_path);
    kfree(parent_path);

    if (!parent) {
        return FS_STATUS_ERROR_NO_ENTRY;
    }

    if (!ramfs_is_directory(parent)) {
        return FS_STATUS_ERROR_NOT_DIRECTORY;
    }

    *out_parent = parent;
    return FS_STATUS_OK;
}

static ramfs_file_t *_ramfs_create_node(char *path, const void *data, size_t size,
                                        uint8_t metadata) {
    ramfs_file_t *node = (ramfs_file_t *)kmalloc(sizeof(ramfs_file_t));
    if (!node) {
        kfree(path);
        return NULL;
    }

    node->name = path;
    if (size > 0) {
        if (!data) {
            kfree(node);
            kfree(path);
            return NULL;
        }
        node->data = kmalloc(size);
        if (!node->data) {
            kfree(node);
            kfree(path);
            return NULL;
        }
        kmemcpy(node->data, data, size);
        node->size = size;
    } else {
        node->data = NULL;
        node->size = 0;
    }
    node->metadata = metadata;
    node->parent = NULL;
    node->child = NULL;
    node->sibling = NULL;
    node->global_next = NULL;
    node->hash_next = NULL;

    return node;
}

static void _ramfs_free_node(ramfs_file_t *node) {
    if (!node) {
        return;
    }
    if (node->data) {
        kfree(node->data);
    }
    if (node->name) {
        kfree(node->name);
    }
    kfree(node);
}

static size_t _ramfs_directory_size_recursive(ramfs_file_t *dir) {
    size_t total = 0;
    for (ramfs_file_t *child = dir->child; child; child = child->sibling) {
        if (ramfs_is_regular_file(child)) {
            total += child->size;
        } else if (ramfs_is_directory(child)) {
            total += _ramfs_directory_size_recursive(child);
        }
    }
    return total;
}

void ramfs_mount(ramfs_t *fs)   { (void)fs; }
void ramfs_unmount(ramfs_t *fs) { (void)fs; }

ramfs_t *ramfs_create(void) {
    ramfs_t *fs = (ramfs_t *)kmalloc(sizeof(ramfs_t));
    if (!fs) {
        return NULL;
    }

    kmemset(fs, 0, sizeof(ramfs_t));

    char *root_path = (char *)kmalloc(2);
    if (!root_path) {
        kfree(fs);
        return NULL;
    }
    root_path[0] = '/';
    root_path[1] = '\0';

    ramfs_file_t *root = _ramfs_create_node(root_path, NULL, 0,
                                            RAMFS_FILE_TYPE_DIRECTORY | RAMFS_DEFAULT_DIR_PERMS);
    if (!root) {
        kfree(fs);
        return NULL;
    }

    fs->root = root;
    _ramfs_link_global(fs, root);
    _ramfs_insert_bucket(fs, root);

    return fs;
}

void ramfs_destroy(ramfs_t *fs) {
    if (!fs) {
        return;
    }

    ramfs_file_t *current = fs->head;
    while (current) {
        ramfs_file_t *next = current->global_next;
        _ramfs_free_node(current);
        current = next;
    }

    kfree(fs);
}

fs_status_t ramfs_add_file(void *fs_ptr, const char *path, const void *data, size_t size,
                           uint8_t metadata) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (strcmp(normalized, "/") == 0) {
        kfree(normalized);
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (_ramfs_lookup(fs, normalized)) {
        kfree(normalized);
        return FS_STATUS_ERROR_ALREADY_EXISTS;
    }

    if (size > 0 && !data) {
        kfree(normalized);
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *parent = NULL;
    fs_status_t parent_status = _ramfs_parent_for(fs, normalized, &parent);
    if (parent_status != FS_STATUS_OK) {
        kfree(normalized);
        return parent_status;
    }

    if (!ramfs_has_permission(parent, FS_PERM_WRITE)) {
        kfree(normalized);
        return FS_STATUS_ERROR_PERMISSION_DENIED;
    }

    uint8_t permissions = metadata & RAMFS_FILE_PERM_MASK;
    if (permissions == 0) {
        permissions = RAMFS_DEFAULT_FILE_PERMS;
    }

    uint8_t node_metadata = RAMFS_FILE_TYPE_REGULAR | (permissions & RAMFS_FILE_PERM_MASK);

    ramfs_file_t *node = _ramfs_create_node(normalized, data, size, node_metadata);
    if (!node) {
        return FS_STATUS_ERROR_NO_SPACE;
    }

    _ramfs_attach_child(parent, node);
    _ramfs_link_global(fs, node);
    _ramfs_insert_bucket(fs, node);
    fs->total_size += size;

    return FS_STATUS_OK;
}

fs_status_t ramfs_write_file(void *fs_ptr, const char *path, const void *data, size_t size) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *file = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (!file) {
        return FS_STATUS_ERROR_NO_ENTRY;
    }
    if (!ramfs_is_regular_file(file)) {
        return FS_STATUS_ERROR_INVALID_TYPE;
    }
    if (!ramfs_has_permission(file, FS_PERM_WRITE)) {
        return FS_STATUS_ERROR_PERMISSION_DENIED;
    }
    if (size > 0 && !data) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    void *copy = NULL;
    if (size > 0) {
        copy = kmalloc(size);
        if (!copy) {
            return FS_STATUS_ERROR_NO_SPACE;
        }
        kmemcpy(copy, data, size);
    }

    void *old_data = file->data;
    size_t old_size = file->size;

    file->data = copy;
    file->size = size;

    if (old_data) {
        kfree(old_data);
    }

    if (fs->total_size >= old_size) {
        fs->total_size -= old_size;
    } else {
        fs->total_size = 0;
    }
    fs->total_size += size;

    return FS_STATUS_OK;
}

fs_status_t ramfs_read_file(void *fs_ptr, const char *path, const void **out_data,
                            size_t *size) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *file = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (!file) {
        return FS_STATUS_ERROR_NO_ENTRY;
    }
    if (!ramfs_is_regular_file(file)) {
        return FS_STATUS_ERROR_INVALID_TYPE;
    }
    if (!ramfs_has_permission(file, FS_PERM_READ)) {
        return FS_STATUS_ERROR_PERMISSION_DENIED;
    }

    *out_data = file->data;
    if (size) {
        *size = file->size;
    }

    return FS_STATUS_OK;
}

fs_status_t ramfs_remove_file(void *fs_ptr, const char *path) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *file = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (!file) {
        return FS_STATUS_ERROR_NO_ENTRY;
    }
    if (!ramfs_is_regular_file(file)) {
        return FS_STATUS_ERROR_INVALID_TYPE;
    }
    if (!ramfs_has_permission(file, FS_PERM_WRITE)) {
        return FS_STATUS_ERROR_PERMISSION_DENIED;
    }

    ramfs_file_t *parent = file->parent ? file->parent : fs->root;
    if (parent) {
        _ramfs_detach_child(parent, file);
    }
    _ramfs_remove_bucket(fs, file);
    _ramfs_unlink_global(fs, file);

    if (fs->total_size >= file->size) {
        fs->total_size -= file->size;
    } else {
        fs->total_size = 0;
    }

    _ramfs_free_node(file);
    return FS_STATUS_OK;
}

fs_status_t ramfs_file_exists(void *fs_ptr, const char *path, bool *out_exists) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *file = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (file && ramfs_is_regular_file(file)) {
        *out_exists = true;
        return FS_STATUS_OK;
    }

    *out_exists = false;
    return FS_STATUS_ERROR_NO_ENTRY;
}

fs_status_t ramfs_file_size(void *fs_ptr, const char *path, size_t *out_size) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *file = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (!file) {
        return FS_STATUS_ERROR_NO_ENTRY;
    }
    if (!ramfs_is_regular_file(file)) {
        return FS_STATUS_ERROR_INVALID_TYPE;
    }
    if (!ramfs_has_permission(file, FS_PERM_READ)) {
        return FS_STATUS_ERROR_PERMISSION_DENIED;
    }

    *out_size = file->size;
    return FS_STATUS_OK;
}

fs_status_t ramfs_make_directory(void *fs_ptr, const char *path) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (_ramfs_lookup(fs, normalized)) {
        kfree(normalized);
        return FS_STATUS_ERROR_ALREADY_EXISTS;
    }

    ramfs_file_t *parent = NULL;
    fs_status_t parent_status = _ramfs_parent_for(fs, normalized, &parent);
    if (parent_status != FS_STATUS_OK) {
        kfree(normalized);
        return parent_status;
    }

    if (!ramfs_has_permission(parent, FS_PERM_WRITE)) {
        kfree(normalized);
        return FS_STATUS_ERROR_PERMISSION_DENIED;
    }

    uint8_t node_metadata = RAMFS_FILE_TYPE_DIRECTORY | RAMFS_DEFAULT_DIR_PERMS;

    ramfs_file_t *node = _ramfs_create_node(normalized, NULL, 0, node_metadata);
    if (!node) {
        return FS_STATUS_ERROR_NO_SPACE;
    }

    _ramfs_attach_child(parent, node);
    _ramfs_link_global(fs, node);
    _ramfs_insert_bucket(fs, node);

    return FS_STATUS_OK;
}

fs_status_t ramfs_remove_directory(void *fs_ptr, const char *path) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (strcmp(normalized, "/") == 0) {
        kfree(normalized);
        return FS_STATUS_ERROR_PERMISSION_DENIED;
    }

    ramfs_file_t *dir = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (!dir) {
        return FS_STATUS_ERROR_NO_ENTRY;
    }
    if (!ramfs_is_directory(dir)) {
        return FS_STATUS_ERROR_NOT_DIRECTORY;
    }
    if (dir->child) {
        return FS_STATUS_ERROR_DIRECTORY_NOT_EMPTY;
    }
    if (!ramfs_has_permission(dir, FS_PERM_WRITE)) {
        return FS_STATUS_ERROR_PERMISSION_DENIED;
    }

    ramfs_file_t *parent = dir->parent ? dir->parent : fs->root;
    if (parent) {
        _ramfs_detach_child(parent, dir);
    }
    _ramfs_remove_bucket(fs, dir);
    _ramfs_unlink_global(fs, dir);

    _ramfs_free_node(dir);
    return FS_STATUS_OK;
}

fs_status_t ramfs_directory_exists(void *fs_ptr, const char *path, bool *out_exists) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *dir = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (dir && ramfs_is_directory(dir)) {
        *out_exists = true;
        return FS_STATUS_OK;
    }

    *out_exists = false;
    return FS_STATUS_ERROR_NO_ENTRY;
}

fs_status_t ramfs_directory_size(void *fs_ptr, const char *path, size_t *out_size) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *dir = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (!dir) {
        return FS_STATUS_ERROR_NO_ENTRY;
    }
    if (!ramfs_is_directory(dir)) {
        return FS_STATUS_ERROR_NOT_DIRECTORY;
    }
    if (!ramfs_has_permission(dir, FS_PERM_READ)) {
        return FS_STATUS_ERROR_PERMISSION_DENIED;
    }

    *out_size = _ramfs_directory_size_recursive(dir);
    return FS_STATUS_OK;
}

fs_status_t ramfs_list_directory(void *fs_ptr, const char *path, char ***out_list,
                                 size_t max_entries, size_t *out_count) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path ? path : "");
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *dir = _ramfs_lookup(fs, normalized);
    if (!dir) {
        kfree(normalized);
        return FS_STATUS_ERROR_NO_ENTRY;
    }
    if (!ramfs_is_directory(dir)) {
        kfree(normalized);
        return FS_STATUS_ERROR_NOT_DIRECTORY;
    }
    if (!ramfs_has_permission(dir, FS_PERM_READ)) {
        kfree(normalized);
        return FS_STATUS_ERROR_PERMISSION_DENIED;
    }

    char **list = (char **)kmalloc(sizeof(char *) * max_entries);
    if (!list) {
        kfree(normalized);
        return FS_STATUS_ERROR_NO_SPACE;
    }

    size_t count = 0;
    for (ramfs_file_t *child = dir->child; child && count < max_entries;
         child = child->sibling) {
        const char *base = _ramfs_basename(child->name);
        size_t len = strlen(base);

        char *entry = (char *)kmalloc(len + 1);
        if (!entry) {
            for (size_t i = 0; i < count; i++) {
                kfree(list[i]);
            }
            kfree(list);
            kfree(normalized);
            return FS_STATUS_ERROR_NO_SPACE;
        }

        kmemcpy(entry, base, len + 1);
        list[count++] = entry;
    }

    kfree(normalized);
    *out_list = list;
    *out_count = count;
    return FS_STATUS_OK;
}

fs_status_t ramfs_get_file_metadata(void *fs_ptr, const char *path, uint8_t *out_metadata) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *file = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (!file) {
        return FS_STATUS_ERROR_NO_ENTRY;
    }

    *out_metadata = file->metadata;
    return FS_STATUS_OK;
}

fs_status_t ramfs_set_file_permissions(void *fs_ptr, const char *path, uint8_t permissions) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *file = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (!file) {
        return FS_STATUS_ERROR_NO_ENTRY;
    }

    file->metadata = (file->metadata & RAMFS_FILE_TYPE_MASK) |
                     (permissions & RAMFS_FILE_PERM_MASK);
    return FS_STATUS_OK;
}

fs_status_t ramfs_get_file_permissions(void *fs_ptr, const char *path,
                                       uint8_t *out_permissions) {
    ramfs_t *fs = (ramfs_t *)fs_ptr;

    char *normalized = _ramfs_normalize_path(path);
    if (!normalized) {
        return FS_STATUS_ERROR_INVALID_ARGUMENT;
    }

    ramfs_file_t *file = _ramfs_lookup(fs, normalized);
    kfree(normalized);

    if (!file) {
        return FS_STATUS_ERROR_NO_ENTRY;
    }

    *out_permissions = file->metadata & RAMFS_FILE_PERM_MASK;
    return FS_STATUS_OK;
}

fs_backend_t ramfs_backend = {
    .create = (void*(*)(void))ramfs_create,
    .destroy = (void(*)(void*))ramfs_destroy,
    .mount = (void(*)(void*))ramfs_mount,
    .unmount = (void(*)(void*))ramfs_unmount,
    .add_file = ramfs_add_file,
    .read_file = ramfs_read_file,
    .remove_file = ramfs_remove_file,
    .write_file = ramfs_write_file,
    .file_exists = ramfs_file_exists,
    .file_size = ramfs_file_size,
    .make_directory = ramfs_make_directory,
    .remove_directory = ramfs_remove_directory,
    .directory_exists = ramfs_directory_exists,
    .directory_size = ramfs_directory_size,
    .list_directory = ramfs_list_directory,
    .get_file_metadata = ramfs_get_file_metadata,
    .set_file_permissions = ramfs_set_file_permissions,
    .get_file_permissions = ramfs_get_file_permissions
};