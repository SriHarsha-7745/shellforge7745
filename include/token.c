#include <stdlib.h>
#include <string.h>
#include "token.h"

Token *create_token(TokenType type, const char *value)
{
    Token *token = malloc(sizeof(Token));

    if (!token)
        return NULL;

    token->type = type;
    token->value = malloc(strlen(value) + 1);

    if (!token->value) {
        free(token);
        return NULL;
    }

    strcpy(token->value, value);

    return token;
}

void free_token(Token *token)
{
    if (token) {
        free(token->value);
        free(token);
    }
}
