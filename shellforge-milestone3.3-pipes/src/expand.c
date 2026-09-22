#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "expand.h"

static void expand_string(char *str) {
    char buffer[MAX_TOKEN_LEN];
    int bi = 0;
    int i = 0;

    while (str[i] != '\0' && bi < MAX_TOKEN_LEN - 1) {
        if (str[i] == '$' && (isalpha((unsigned char)str[i + 1]) || str[i + 1] == '_')) {
            int j = i + 1;
            char varname[128];
            int vi = 0;

            while (str[j] != '\0' && (isalnum((unsigned char)str[j]) || str[j] == '_') && vi < 127) {
                varname[vi++] = str[j++];
            }
            varname[vi] = '\0';

            const char *val = getenv(varname);
            if (val != NULL) {
                for (int k = 0; val[k] != '\0' && bi < MAX_TOKEN_LEN - 1; k++) {
                    buffer[bi++] = val[k];
                }
            }
            i = j;
        } else {
            buffer[bi++] = str[i++];
        }
    }

    buffer[bi] = '\0';
    strncpy(str, buffer, MAX_TOKEN_LEN - 1);
    str[MAX_TOKEN_LEN - 1] = '\0';
}

void expand_variables(pipeline_t *pipeline) {
    for (int i = 0; i < pipeline->command_count; i++) {
        command_t *cmd = &pipeline->commands[i];

        for (int j = 0; j < cmd->argc; j++) {
            expand_string(cmd->argv[j]);
        }
        if (cmd->input[0] != '\0')  expand_string(cmd->input);
        if (cmd->output[0] != '\0') expand_string(cmd->output);
    }
}
