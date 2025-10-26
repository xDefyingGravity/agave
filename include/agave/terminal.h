#ifndef AGAVE_TERMINAL_H
#define AGAVE_TERMINAL_H

#include <stdbool.h>

#define MAX_COMMAND_LEN 4096
#define HISTORY_SIZE 256

extern char history[HISTORY_SIZE][MAX_COMMAND_LEN];
extern int history_count;

void terminal_initialize(bool show_prompt);

#endif // AGAVE_TERMINAL_H