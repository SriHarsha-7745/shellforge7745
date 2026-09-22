#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "builtin.h"
#include "executor.h"

int main(void) {
    token_list_t tokens;
    pipeline_t pipeline;
    char *line;
    int should_exit = 0;

    printf("========================================\n");
    printf("    Shellforge\n");
    printf(" A Unix Style Shell written in C\n");
    printf("========================================\n");

    setup_background_handler();

    while (1) {
        line = readline("shellforge$ ");

        if (line == NULL) {
            printf("\nGoodbye!\n");
            break;
        }

        if (strlen(line) == 0) {
            free(line);
            continue;
        }

        add_history(line);

        lexer(line, &tokens);
        token_print(&tokens);

        if (parse(&tokens, &pipeline)) {
            expand_variables(&pipeline);
            pipeline_print(&pipeline);

            execute_pipeline(&pipeline);

            /* "exit" is only recognized when it's the sole command
             * typed at the prompt -- matches how execute_builtin's
             * exit-signal (return 1) is meant to be used */
            if (pipeline.command_count == 1 &&
                pipeline.commands[0].argc > 0 &&
                strcmp(pipeline.commands[0].argv[0], "exit") == 0) {
                should_exit = 1;
            }
        }

        free(line);

        if (should_exit) {
            break;
        }
    }

    return 0;
}
