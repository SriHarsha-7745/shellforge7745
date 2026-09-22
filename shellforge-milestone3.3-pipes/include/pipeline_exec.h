#ifndef PIPELINE_EXEC_H
#define PIPELINE_EXEC_H

#include "parser.h"

/* Runs an entire pipeline (one or more commands joined by |).
 * Returns 1 if the shell should exit (i.e. "exit" was run), else 0. */
int execute_pipeline(pipeline_t *pipeline);

#endif
