#include <agave/kvid.h>
#include <agave/terminal.h>
#include <agave/kcore.h>
#include <agave/klog.h>
#include <agave/term/command.h>
#include <string.h>

typedef void (*command_fn)(const char *args, command_output_fn out);

typedef struct {
  const char *name;
  command_fn func;
} command_entry_t;

static void cmd_echo(const char *args, command_output_fn out) { out(args); }

static void cmd_shutdown(const char *args, command_output_fn out) {
  (void)args;
  out("shutting down the system...\n");
  kshutdown();
}

static void cmd_panic(const char *args, command_output_fn out) {
  out("triggering kernel panic...\n");
  kpanic("manual panic triggered by command: %s", args);
}

static void cmd_bootinfo(const char *args, command_output_fn out) {
  (void)args;
  kcore_information_t *info = kcore_get_information();
  out("kernel version: %s\n", info->kernel_version);
  out("cpu count: %u\n", info->cpus_count);
  out("uptime (ticks): %u\n", info->uptime_ticks);
}

static void cmd_uptime(const char *args, command_output_fn out) {
  (void)args;
  kcore_information_t *info = kcore_get_information();
  out("system uptime: %u ticks\n", info->uptime_ticks);
}

static void cmd_clear(const char *args, command_output_fn out) {
  (void)args;
  (void)out;
  kclear();
  terminal_initialize(false);
}

static void cmd_history(const char *args, command_output_fn out) {
    (void)args;
    for (int i = 0; i < history_count   ; i++) {
        out("%d: %s\n", i + 1, history[i]);
    }
}

static void cmd_version(const char *args, command_output_fn out) {
  (void)args;
  kcore_information_t *info = kcore_get_information();
  out("kernel version: %s\n", info->kernel_version);
}

static void cmd_help(const char *args, command_output_fn out) {
  (void)args;
  out("Available commands:\n"
      "  echo [text]     - prints the provided text.\n"
      "  shutdown        - shuts down the system.\n"
      "  panic           - triggers a kernel panic.\n"
      "  help            - displays this help message.\n"
      "  bootinfo        - displays boot information.\n"
      "  uptime          - shows system uptime in ticks.\n"
      "  clear           - clears the terminal screen.\n"
      "  history         - shows command history.\n"
      "  version         - displays kernel version information.\n");
}

static const command_entry_t commands[] = {
    {"echo", cmd_echo},       {"shutdown", cmd_shutdown},
    {"panic", cmd_panic},     {"bootinfo", cmd_bootinfo},
    {"uptime", cmd_uptime},   {"clear", cmd_clear},
    {"help", cmd_help},       {"history", cmd_history},
    {"version", cmd_version}, {NULL, NULL}
};

void execute_command(const char *command_line, command_output_fn out) {
  const char *space = strchr(command_line, ' ');
  size_t cmd_len =
      space ? (size_t)(space - command_line) : strlen(command_line);

  for (const command_entry_t *cmd = commands; cmd->name != NULL; cmd++) {
    if (strncmp(cmd->name, command_line, cmd_len) == 0 &&
        strlen(cmd->name) == cmd_len) {
      const char *args = space ? space + 1 : "";
      cmd->func(args, out);
      return;
    }
  }

  kerror("unknown command: %s\n", command_line);
}