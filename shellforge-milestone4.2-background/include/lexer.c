#include <stdio.h>
#include <ctype.h>
#include "lexer.h"

void lexer(const char *input, token_list_t *list) {
    token_list_init(list);
    int i = 0;

    while (1) {
        if (input[i] == '\0') {
            token_add(list, TOKEN_END, "END");
            break;
        }

        if (isspace((unsigned char)input[i])) {
            i++;
            continue;
        }

        if (input[i] == '|') {
            token_add(list, TOKEN_PIPE, "|");
            i++;
            continue;
        }

        if (input[i] == '<') {
            token_add(list, TOKEN_INPUT, "<");
            i++;
            continue;
        }

        if (input[i] == '>') {
            if (input[i + 1] == '>') {
                token_add(list, TOKEN_APPEND, ">>");
                i += 2;
            } else {
                token_add(list, TOKEN_OUTPUT, ">");
                i++;
            }
            continue;
        }

        if (input[i] == '&') {
            token_add(list, TOKEN_BACKGROUND, "&");
            i++;
            continue;
        }

        /* Build a WORD token */
        char word[MAX_TOKEN_LEN];
        int j = 0;

        while (input[i] != '\0' && !isspace((unsigned char)input[i]) &&
               input[i] != '|' && input[i] != '<' &&
               input[i] != '>' && input[i] != '&') {

            char c = input[i];

            if (c == '\'') {
                i++; /* skip opening ' */
                while (input[i] != '\0' && input[i] != '\'') {
                    if (j < MAX_TOKEN_LEN - 1) word[j++] = input[i];
                    i++;
                }
                if (input[i] == '\0') {
                    fprintf(stderr, "lexer error: unterminated single quote\n");
                    return;
                }
                i++; /* skip closing ' */
            } else if (c == '"') {
                i++; /* skip opening " */
                while (input[i] != '\0' && input[i] != '"') {
                    if (input[i] == '\\' && input[i + 1] != '\0') {
                        i++;
                    }
                    if (j < MAX_TOKEN_LEN - 1) word[j++] = input[i];
                    i++;
                }
                if (input[i] == '\0') {
                    fprintf(stderr, "lexer error: unterminated double quote\n");
                    return;
                }
                i++; /* skip closing " */
            } else if (c == '\\') {
                i++; /* skip backslash */
                if (input[i] != '\0') {
                    if (j < MAX_TOKEN_LEN - 1) word[j++] = input[i];
                    i++;
                }
            } else {
                if (j < MAX_TOKEN_LEN - 1) word[j++] = c;
                i++;
            }
        }

        word[j] = '\0';
        token_add(list, TOKEN_WORD, word);
    }
}
