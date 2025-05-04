#include "sql-token.h"

#include <stdlib.h>

Token *create_token(char *lexeme, 
    size_t lexeme_len, 
    TokenType type, 
    size_t row, 
    size_t symbol
) {
    Token *token = malloc(sizeof(Token));
    if (!token) {
        printf("Failed to allocate memory for a new token\n");
        perror("Error");
        return NULL;
    }
    token->lexeme = lexeme;
    token->lexeme_len = lexeme_len;
    token->type = type;
    token->row = row;
    token->symbol = symbol;

    return token;
}
