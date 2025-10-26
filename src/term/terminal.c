#include <agave/term/command.h>
#include <agave/input.h>
#include <agave/keys.h>
#include <agave/kvid.h>
#include <agave/terminal.h>
#include <stdbool.h>
#include <string.h>

static char command_buffer[MAX_COMMAND_LEN];
static int command_length = 0;
static int cursor_pos = 0;

char history[HISTORY_SIZE][MAX_COMMAND_LEN];
int history_count = 0;
static int history_index = -1;

static void push_history(const char *cmd) {
  if (history_count < HISTORY_SIZE) {
    strcpy(history[history_count++], cmd);
  } else {
    for (int i = 1; i < HISTORY_SIZE; i++)
      strcpy(history[i - 1], history[i]);
    strcpy(history[HISTORY_SIZE - 1], cmd);
  }
}

static void redraw_line(void) {
  size_t krow = kgetpos().krow;

  ksetpos(krow, 0);
  kprintf("> %s", command_buffer);

  for (int i = command_length + 2; i < kline_end[krow]; i++)
    vidptr[krow * SCREEN_WIDTH + i] = (kcurrent_color << 8) | ' ';
  kline_end[krow] = command_length + 2;

  ksetpos(krow, cursor_pos + 2);
}

bool terminal_key_press(uint8_t c) {
  size_t krow = kgetpos().krow;

  switch (c) {
  case KEY_BACKSPACE:
    if (cursor_pos > 0) {
      cursor_pos--;
      command_length--;
      for (int i = cursor_pos; i < command_length; i++)
        command_buffer[i] = command_buffer[i + 1];
      command_buffer[command_length] = 0;
      redraw_line();
    }
    return true;

  case KEY_LEFT:
    if (cursor_pos > 0)
      cursor_pos--;
    ksetpos(krow, cursor_pos + 2);
    return true;

  case KEY_RIGHT:
    if (cursor_pos < command_length)
      cursor_pos++;
    ksetpos(krow, cursor_pos + 2);
    return true;

  case KEY_UP:
    if (history_count > 0 && history_index + 1 < history_count) {
      history_index++;
      strcpy(command_buffer, history[history_count - 1 - history_index]);
      command_length = strlen(command_buffer);
      cursor_pos = command_length;
      redraw_line();
    }
    return true;

  case KEY_DOWN:
    if (history_index > 0) {
      history_index--;
      strcpy(command_buffer, history[history_count - 1 - history_index]);
      command_length = strlen(command_buffer);
      cursor_pos = command_length;
      redraw_line();
    } else if (history_index == 0) {
      history_index = -1;
      command_length = 0;
      cursor_pos = 0;
      command_buffer[0] = 0;
      redraw_line();
    }
    return true;

  case KEY_ENTER:
  {
    kprintf("\n");

    if (command_length > 0) {
      push_history(command_buffer);

      execute_command(command_buffer, kprintf);
    }

    command_length = 0;
    cursor_pos = 0;
    history_index = -1;
    command_buffer[0] = '\0';

    kprintf("> ");
    return true;
  }
  default:
    if (c >= KEY_SPACE && c <= KEY_TILDE &&
        command_length < MAX_COMMAND_LEN - 1) {
      for (int i = command_length; i > cursor_pos; i--)
        command_buffer[i] = command_buffer[i - 1];
      command_buffer[cursor_pos++] = c;
      command_length++;
      command_buffer[command_length] = 0;
      redraw_line();
    }
    return true;
  }
}

void terminal_key_release(uint8_t c) { (void)c; }

static void print_center_text(const char *text) {
  size_t len = strlen(text);
  size_t left_padding = (SCREEN_WIDTH > len) ? (SCREEN_WIDTH - len) / 2 : 0;
  for (size_t i = 0; i < left_padding; i++)
    kputchar(' ');
  kprint(text);
  kputchar('\n');
}

void terminal_initialize(bool show_prompt) {
    REGISTER_INPUT_HOOK(terminal_key_press, terminal_key_release);

    kclear();
    command_length = 0;
    cursor_pos = 0;
    command_buffer[0] = '\0';

    print_center_text("agave os terminal interface - v0.1");
    print_center_text("type 'help' for a list of commands.");
    if (show_prompt)
        kprintf("\n> ");
}