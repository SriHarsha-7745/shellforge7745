#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "token.h"

void lexer(const char *input, token_list_t *list)
{
    int i = 0;

    token_list_init(list);

    while (input[i] != '\0')
    {
        /* Skip whitespace */
        if (isspace((unsigned char)input[i]))
        {
            i++;
            continue;
        }

        /* Pipe */
        if (input[i] == '|')
        {
            token_add(list, TOKEN_PIPE, "|");
            i++;
            continue;
        }

        /* Input redirect */
        if (input[i] == '<')
        {
            token_add(list, TOKEN_INPUT, "<");
            i++;
            continue;
        }

        /* Output redirect / Append */
        if (input[i] == '>')
        {
            if (input[i + 1] == '>')
            {
                token_add(list, TOKEN_APPEND, ">>");
                i += 2;
            }
            else
            {
                token_add(list, TOKEN_OUTPUT, ">");
                i++;
            }
            continue;
        }

        /* Background */
        if (input[i] == '&')
        {
            token_add(list, TOKEN_BACKGROUND, "&");
            i++;
            continue;
        }

        /* Build WORD token */
        char word[MAX_TOKEN_LEN];
        int j = 0;

        while (input[i] != '\0' &&
               !isspace((unsigned char)input[i]) &&
               input[i] != '|' &&
               input[i] != '<' &&
               input[i] != '>' &&
               input[i] != '&')
        {
            /* Single quoted string */
            if (input[i] == '\'')
            {
                i++;

                while (input[i] != '\0' && input[i] != '\'')
                {
                    if (j < MAX_TOKEN_LEN - 1)
                        word[j++] = input[i];

                    i++;
                }

                if (input[i] == '\'')
                    i++;

                continue;
            }

            /* Double quoted string */
            if (input[i] == '"')
            {
                i++;

                while (input[i] != '\0' && input[i] != '"')
                {
                    if (input[i] == '\\' && input[i + 1] != '\0')
                        i++;

                    if (j < MAX_TOKEN_LEN - 1)
                        word[j++] = input[i];

                    i++;
                }

                if (input[i] == '"')
                    i++;

                continue;
            }

            /* Escape character */
            if (input[i] == '\\')
            {
                i++;

                if (input[i] != '\0')
                {
                    if (j < MAX_TOKEN_LEN - 1)
                        word[j++] = input[i];

                    i++;
                }

                continue;
            }

            /* Normal character */
            if (j < MAX_TOKEN_LEN - 1)
                word[j++] = input[i];

            i++;
        }

        word[j] = '\0';

        if (j > 0)
            token_add(list, TOKEN_WORD, word);
    }

    /* Add END token */
    token_add(list, TOKEN_END, "END");
}
