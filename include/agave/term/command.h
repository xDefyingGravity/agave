#ifndef AGAVE_TERM_COMMAND_H
#define AGAVE_TERM_COMMAND_H

typedef void (*command_output_fn)(const char *text, ...);

void execute_command(const char *command_line, command_output_fn out);

#endif // AGAVE_TERM_COMMAND_H