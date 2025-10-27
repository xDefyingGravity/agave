#include <agave/kmem.h>
#include <agave/fs.h>
#include <agave/kcore.h>
#include <agave/klog.h>
#include <agave/terminal.h>
#include <string.h>
#include <stdbool.h>

typedef void (*command_output_fn)(const char *fmt, ...);
typedef void (*command_fn)(const char *args, command_output_fn out);

typedef struct {
    const char *name;
    const char *description;
    command_fn func;
} command_entry_t;

#define COMMAND(name, desc) \
    static void cmd_##name(const char *args, command_output_fn out); \
    static const command_entry_t _cmd_##name __attribute__((used, section("commands"))) = {#name, desc, cmd_##name}; \
    static void cmd_##name(const char *args, command_output_fn out)

extern const command_entry_t __start_commands[];
extern const command_entry_t __stop_commands[];

static inline const char *arg_skip_whitespace(const char *args) {
    while (*args == ' ' || *args == '\t') args++;
    return args;
}

static inline const char *arg_next(const char **args, size_t *len) {
    *args = arg_skip_whitespace(*args);
    if (!**args) {
        *len = 0;
        return NULL;
    }
    const char *start = *args;
    while (**args && **args != ' ' && **args != '\t') (*args)++;
    *len = (size_t)(*args - start);
    return start;
}

static inline const char *arg_rest(const char *args) {
    return arg_skip_whitespace(args);
}

void execute_command(const char *command_line, command_output_fn out) {
    const char *space = strchr(command_line, ' ');
    size_t cmd_len = space ? (size_t)(space - command_line) : strlen(command_line);

    for (const command_entry_t *cmd = __start_commands; cmd < __stop_commands; cmd++) {
        if (strncmp(cmd->name, command_line, cmd_len) == 0 && strlen(cmd->name) == cmd_len) {
            const char *args = space ? space + 1 : "";
            cmd->func(args, out);
            return;
        }
    }

    out("command not found: %.*s\n", (int)cmd_len, command_line);
}

COMMAND(echo, "prints the provided text") {
    out("%s\n", arg_rest(args));
}

COMMAND(shutdown, "shuts down the system") {
    (void)args;
    out("shutting down...\n");
    kshutdown();
}

COMMAND(panic, "triggers a kernel panic") {
    out("triggering kernel panic...\n");
    kpanic("manual panic triggered by command: %s", args);
}

COMMAND(bootinfo, "displays boot information") {
    (void)args;
    kcore_information_t *info = kcore_get_information();
    out("kernel version: %s\n", info->kernel_version);
    out("cpu count: %u\n", info->cpus_count);
    out("uptime (ticks): %u\n", info->uptime_ticks);
}

COMMAND(ls, "lists files in the specified directory") {
    fs_t *fs = kcore_get_information()->fs;
    if (!fs || !fs_is_mounted(fs)) { out("no filesystem mounted.\n"); return; }
    const char *dir = args[0] ? arg_rest(args) : "/";
    char **list = NULL;
    size_t count = 0;
    fs_status_t status = fs_list_directory(fs, dir, &list, 256, &count);
    if (status != FS_STATUS_OK) {
        out("error listing directory '%s': %s\n", dir, fs_status_to_string(status));
        return;
    }
    for (size_t i = 0; i < count; i++) {
        out("%s\n", list[i]);
        kfree(list[i]);
    }
    kfree(list);
}

COMMAND(touch, "creates an empty file") {
    fs_t *fs = kcore_get_information()->fs;
    if (!fs || !fs_is_mounted(fs)) { out("no filesystem mounted.\n"); return; }

    size_t len;
    const char *file = arg_next(&args, &len);
    if (!file || !len) { out("usage: touch [filename]\n"); return; }

    char *filename = (char *)kmalloc(len + 1);
    kmemcpy(filename, file, len);
    filename[len] = '\0';

    fs_status_t status = fs_add_file(fs, filename, NULL, 0,
                                     RAMFS_FILE_TYPE_REGULAR | FS_PERM_READ | FS_PERM_WRITE);
    kfree(filename);

    if (status != FS_STATUS_OK) { out("error creating file: %s\n", fs_status_to_string(status)); return; }
    out("file created successfully.\n");
}

COMMAND(cat, "displays the contents of a file") {
    fs_t *fs = kcore_get_information()->fs;
    if (!fs || !fs_is_mounted(fs)) { out("no filesystem mounted.\n"); return; }

    size_t len;
    const char *file = arg_next(&args, &len);
    if (!file || !len) { out("usage: cat [filename]\n"); return; }

    char *filename = (char *)kmalloc(len + 1);
    kmemcpy(filename, file, len);
    filename[len] = '\0';

    const void *data = NULL;
    size_t size = 0;
    fs_status_t status = fs_read_file(fs, filename, &data, &size);
    kfree(filename);

    if (status != FS_STATUS_OK) { out("error reading file: %s\n", fs_status_to_string(status)); return; }
    out("%.*s\n", (int)size, (const char *)data);
}

COMMAND(writeto, "writes text to a file") {
    fs_t *fs = kcore_get_information()->fs;
    if (!fs || !fs_is_mounted(fs)) { out("no filesystem mounted.\n"); return; }

    size_t len;
    const char *file = arg_next(&args, &len);
    if (!file || !len) { out("usage: writeto [filename] [text]\n"); return; }

    char *filename = (char *)kmalloc(len + 1);
    kmemcpy(filename, file, len);
    filename[len] = '\0';

    const char *text = arg_rest(args);
    size_t text_len = strlen(text);

    fs_status_t status = fs_write_file(fs, filename, text, text_len);
    kfree(filename);

    if (status != FS_STATUS_OK) { out("error writing to file: %s\n", fs_status_to_string(status)); return; }
    out("wrote to file successfully.\n", text_len);
}

COMMAND(mkdir, "creates a new directory") {
    fs_t *fs = kcore_get_information()->fs;
    if (!fs || !fs_is_mounted(fs)) { out("no filesystem mounted.\n"); return; }

    size_t len;
    const char *dir = arg_next(&args, &len);
    if (!dir || !len) { out("usage: mkdir [directory]\n"); return; }

    char *dirname = (char *)kmalloc(len + 1);
    kmemcpy(dirname, dir, len);
    dirname[len] = '\0';

    fs_status_t status = fs_make_directory(fs, dirname);
    kfree(dirname);

    if (status != FS_STATUS_OK) { out("error creating directory: %s\n", fs_status_to_string(status)); return; }
    out("directory created successfully.\n");
}

COMMAND(rmdir, "removes a directory") {
    fs_t *fs = kcore_get_information()->fs;
    if (!fs || !fs_is_mounted(fs)) { out("no filesystem mounted.\n"); return; }

    size_t len;
    const char *dir = arg_next(&args, &len);
    if (!dir || !len) { out("usage: rmdir [directory]\n"); return; }

    char *dirname = (char *)kmalloc(len + 1);
    kmemcpy(dirname, dir, len);
    dirname[len] = '\0';

    fs_status_t status = fs_remove_directory(fs, dirname);
    kfree(dirname);

    if (status != FS_STATUS_OK) { out("error removing directory: %s\n", fs_status_to_string(status)); return; }
    out("directory removed successfully.\n");
}

COMMAND(help, "lists all available commands") {
    (void)args;
    out("Available commands:\n");
    for (const command_entry_t *cmd = __start_commands; cmd < __stop_commands; cmd++) {
        out("  %s: %s\n", cmd->name, cmd->description);
    }
}