#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "token.h"
#include "lexer.h"

int main(void) {
    char *line;
    token_list_t list;

    printf("========================================\n");
    printf("    Shellforge\n");
    printf(" A Unix Style Shell written in C\n");
    printf("========================================\n");

    while ((line = readline("shellforge$ ")) != NULL) {
        if (strlen(line) == 0) {
            free(line);
            continue;
        }

        add_history(line);

        if (strcmp(line, "exit") == 0) {
            printf("Exiting...\n");
            free(line);
            break;
        }

        if (strcmp(line, "history") == 0) {
            HIST_ENTRY **hist = history_list();
            printf("------ Command History ------\n");
            if (hist) {
                for (int i = 0; hist[i]; i++) {
                    printf(" %2d  %s\n", i + 1, hist[i]->line);
                }
            }
            printf("------------------------------\n");
            free(line);
            continue;
        }

        lexer(line, &list);
        token_print(&list);

        free(line);
    }

    return 0;
}
