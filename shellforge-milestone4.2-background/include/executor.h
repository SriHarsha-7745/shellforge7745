#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

/* Executes a single command_t: builtin, or external via fork()+execvp().
 * Returns -1 if cmd is NULL/empty, else the exit status of the command
 * (or 0 immediately if cmd->background is set -- see below). */
int execute_command(command_t *cmd);

/* Executes an entire pipeline_t (one or more commands joined by |).
 * Returns -1 if pipeline is NULL/empty, else the exit status of the
 * last command in the pipeline (or 0 immediately if the last command's
 * background flag is set). */
int execute_pipeline(pipeline_t *pipeline);

/* Installs the SIGCHLD handler that reaps finished background jobs so
 * they don't linger as zombies. Call this once, at shell startup. */
void setup_background_handler(void);

#endif

