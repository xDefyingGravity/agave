#include <agave/fs.h>
#include <agave/kcore.h>
#include <agave/kmem.h>
#include <string.h>

static fs_t *mounted_fs[MAX_FS];
static size_t fs_count = 0;

static void check_fs_integrity(fs_t *fs) {
#define CONFIRM_BACKEND_METHOD(method)                                         \
  if (!fs->backend->method) {                                                  \
    kpanic("filesystem backend for '%s' missing method: " #method, fs->name);  \
  }

  if (!fs || !fs->backend) {
    kpanic("invalid filesystem structure");
  }

  CONFIRM_BACKEND_METHOD(create);
  CONFIRM_BACKEND_METHOD(destroy);
  CONFIRM_BACKEND_METHOD(mount);
  CONFIRM_BACKEND_METHOD(unmount);
  CONFIRM_BACKEND_METHOD(add_file);
  CONFIRM_BACKEND_METHOD(read_file);
  CONFIRM_BACKEND_METHOD(remove_file);
  CONFIRM_BACKEND_METHOD(write_file);
  CONFIRM_BACKEND_METHOD(file_exists);
  CONFIRM_BACKEND_METHOD(file_size);
  CONFIRM_BACKEND_METHOD(make_directory);
  CONFIRM_BACKEND_METHOD(remove_directory);
  CONFIRM_BACKEND_METHOD(directory_exists);
  CONFIRM_BACKEND_METHOD(directory_size);
  CONFIRM_BACKEND_METHOD(list_directory);
  CONFIRM_BACKEND_METHOD(get_file_metadata);
  CONFIRM_BACKEND_METHOD(set_file_permissions);
  CONFIRM_BACKEND_METHOD(get_file_permissions);
#undef CONFIRM_BACKEND_METHOD
}

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

  check_fs_integrity(fs);
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

fs_t **fs_get_mounted(size_t *count) {
  if (count)
    *count = fs_count;
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

static fs_status_t ensure_valid_fs(fs_t *fs) {
  if (!fs || !fs->backend || !fs->backend_data) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }
  return FS_STATUS_OK;
}

static fs_status_t ensure_valid_path(const char *path, bool allow_empty) {
  if (!path) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }
  if (!allow_empty && path[0] == '\0') {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }
  return FS_STATUS_OK;
}

static fs_status_t ensure_writable(fs_t *fs) {
  if (fs_is_readonly(fs)) {
    return FS_STATUS_ERROR_READ_ONLY;
  }
  return FS_STATUS_OK;
}

fs_status_t fs_add_file(fs_t *fs, const char *path, const void *data,
                        size_t size, uint8_t metadata) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_writable(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, true);
  if (status != FS_STATUS_OK)
    return status;

  if (size > 0 && !data) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }

  return fs->backend->add_file(fs->backend_data, path, data, size, metadata);
}

fs_status_t fs_read_file(fs_t *fs, const char *path, const void **out_data,
                         size_t *out_size) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, false);
  if (status != FS_STATUS_OK)
    return status;

  if (!out_data) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }

  return fs->backend->read_file(fs->backend_data, path, out_data, out_size);
}

fs_status_t fs_remove_file(fs_t *fs, const char *path) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_writable(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, false);
  if (status != FS_STATUS_OK)
    return status;

  return fs->backend->remove_file(fs->backend_data, path);
}

fs_status_t fs_write_file(fs_t *fs, const char *path, const void *data,
                          size_t size) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_writable(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, false);
  if (status != FS_STATUS_OK)
    return status;

  if (size > 0 && !data) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }

  return fs->backend->write_file(fs->backend_data, path, data, size);
}

fs_status_t fs_file_exists(fs_t *fs, const char *path, bool *out_exists) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, false);
  if (status != FS_STATUS_OK)
    return status;

  if (!out_exists) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }

  return fs->backend->file_exists(fs->backend_data, path, out_exists);
}

fs_status_t fs_file_size(fs_t *fs, const char *path, size_t *out_size) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, false);
  if (status != FS_STATUS_OK)
    return status;

  if (!out_size) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }

  return fs->backend->file_size(fs->backend_data, path, out_size);
}

fs_status_t fs_make_directory(fs_t *fs, const char *path) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_writable(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, true);
  if (status != FS_STATUS_OK)
    return status;

  return fs->backend->make_directory(fs->backend_data, path);
}

fs_status_t fs_remove_directory(fs_t *fs, const char *path) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_writable(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, true);
  if (status != FS_STATUS_OK)
    return status;

  return fs->backend->remove_directory(fs->backend_data, path);
}

fs_status_t fs_directory_exists(fs_t *fs, const char *path, bool *out_exists) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, true);
  if (status != FS_STATUS_OK)
    return status;

  if (!out_exists) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }

  return fs->backend->directory_exists(fs->backend_data, path, out_exists);
}

fs_status_t fs_directory_size(fs_t *fs, const char *path, size_t *out_size) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, false);
  if (status != FS_STATUS_OK)
    return status;

  if (!out_size) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }

  return fs->backend->directory_size(fs->backend_data, path, out_size);
}

fs_status_t fs_list_directory(fs_t *fs, const char *path, char ***out_list,
                              size_t max_entries, size_t *out_count) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  if (!out_list || !out_count || max_entries == 0) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }

  status = ensure_valid_path(path, false);
  if (status != FS_STATUS_OK)
    return status;

  return fs->backend->list_directory(fs->backend_data, path, out_list,
                                     max_entries, out_count);
}

fs_status_t fs_get_file_metadata(fs_t *fs, const char *path,
                                 uint8_t *out_metadata) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, false);
  if (status != FS_STATUS_OK)
    return status;

  if (!out_metadata) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }

  return fs->backend->get_file_metadata(fs->backend_data, path, out_metadata);
}

fs_status_t fs_set_file_permissions(fs_t *fs, const char *path,
                                    uint8_t permissions) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_writable(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, false);
  if (status != FS_STATUS_OK)
    return status;

  return fs->backend->set_file_permissions(fs->backend_data, path, permissions);
}

fs_status_t fs_get_file_permissions(fs_t *fs, const char *path,
                                    uint8_t *out_permissions) {
  fs_status_t status = ensure_valid_fs(fs);
  if (status != FS_STATUS_OK)
    return status;

  status = ensure_valid_path(path, false);
  if (status != FS_STATUS_OK)
    return status;

  if (!out_permissions) {
    return FS_STATUS_ERROR_INVALID_ARGUMENT;
  }

  return fs->backend->get_file_permissions(fs->backend_data, path,
                                           out_permissions);
}

const char *fs_status_to_string(fs_status_t status) {
  switch (status) {
  case FS_STATUS_OK:
    return "OK";
  case FS_STATUS_ERROR_NO_ENTRY:
    return "FILE_NOT_FOUND";
  case FS_STATUS_ERROR_PERMISSION_DENIED:
    return "PERMISSION_DENIED";
  case FS_STATUS_ERROR_ALREADY_EXISTS:
    return "ALREADY_EXISTS";
  case FS_STATUS_ERROR_INVALID_TYPE:
    return "INVALID_TYPE";
  case FS_STATUS_ERROR_NOT_DIRECTORY:
    return "NOT_A_DIRECTORY";
  case FS_STATUS_ERROR_DIRECTORY_NOT_EMPTY:
    return "DIRECTORY_NOT_EMPTY";
  case FS_STATUS_ERROR_READ_ONLY:
    return "READ_ONLY";
  case FS_STATUS_ERROR_NO_SPACE:
    return "NO_SPACE";
  case FS_STATUS_ERROR_INVALID_ARGUMENT:
    return "INVALID_ARGUMENT";
  case FS_STATUS_ERROR_UNKNOWN:
  default:
    return "UNKNOWN";
  }
}