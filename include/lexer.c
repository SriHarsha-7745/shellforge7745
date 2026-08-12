#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

static int is_symbol(char c)
{
    return c == '|' || c == '<' || c == '>';
}

Token *tokenize(const char *input, int *count)
{
    int capacity = 16;
    int n = 0;

    Token *tokens = malloc(capacity * sizeof(Token));

    if (!tokens)
        return NULL;

    int i = 0;

    while (input[i]) {

        while (isspace((unsigned char)input[i]))
            i++;

        if (!input[i])
            break;

        if (n >= capacity) {
            capacity *= 2;

            Token *temp = realloc(tokens,
                                  capacity * sizeof(Token));

            if (!temp) {
                free_tokens(tokens, n);
                return NULL;
            }

            tokens = temp;
        }

        if (is_symbol(input[i])) {

            tokens[n].type = TOKEN_SYMBOL;
            tokens[n].value = malloc(2);

            if (!tokens[n].value) {
                free_tokens(tokens, n);
                return NULL;
            }

            tokens[n].value[0] = input[i];
            tokens[n].value[1] = '\0';

            n++;
            i++;
        }
        else {

            int start = i;

            while (input[i] &&
                   !isspace((unsigned char)input[i]) &&
                   !is_symbol(input[i])) {
                i++;
            }

            int len = i - start;

            tokens[n].type = TOKEN_WORD;
            tokens[n].value = malloc(len + 1);

            if (!tokens[n].value) {
                free_tokens(tokens, n);
                return NULL;
            }

            memcpy(tokens[n].value,
                   input + start,
                   len);

            tokens[n].value[len] = '\0';

            n++;
        }
    }

    *count = n;
    return tokens;
}

void free_tokens(Token *tokens, int count)
{
    for (int i = 0; i < count; i++)
        free(tokens[i].value);

    free(tokens);
}
