#include "sql-lexing.h"
#include "sql-token.h"
#include <ctype.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include "../../errors.h"

const char space_delim = ' ';

Token **lex(char *command, size_t *out_count, ErrorCode *out_error) {
    out_count = NULL;
    out_error = NULL;

    size_t capacity = 128;
    size_t count = 0;
    size_t i = 0;
    size_t len = strlen(command);

    Token **tokens = malloc(sizeof(Token *) * capacity);

    size_t row, symbol = 0;

    while (i < len) {
        if (command[i] == '\n') {
            symbol = 0;
            row++;
        } else {
            symbol++;
        }

        if (isspace(command[i])) {
            i++; symbol++;
            continue;
        }
        
        if (command[i] == '"') {
            size_t start = ++i;

            while (i < len && command[i] != '"') i++;
            size_t tok_len = i - start;
            char *str = strndup(command + start, tok_len);
            
            Token *token = create_token(str, tok_len, TT_IDENTIFIER, row, symbol);
            
            tokens[count++] = token;
            symbol = i;
        }

        if (command[i] == '\'') {
            size_t start = ++i;

            while (i < len && command[i] != '\'') i++;
            size_t tok_len = i - start;
            char *str = strndup(command + start, tok_len);
            
            Token *token = create_token(str, tok_len, TT_LIT_STRING, row, symbol);

            tokens[count++] = token;
            symbol = i;
        }

        if (command[i] == '(') {
            char *str = malloc(2 * sizeof(char));
            str[0] = '(';
            str[1] = '\0';
            Token *token = create_token(str, 1, TT_SP_LEFT_BRACE, row, symbol);
            tokens[count++] = token;
        }

        if (command[i] == ')') {
            char *str = malloc(2 * sizeof(char));
            str[0] = ')';
            str[1] = '\0';
            Token *token = create_token(str, 1, TT_SP_RIGHT_BRACE, row, symbol);
            tokens[count++] = token;
        }

        if (command[i] == ',') {
            char *str = malloc(2 * sizeof(char));
            str[0] = ',';
            str[1] = '\0';
            Token *token = malloc(sizeof(Token));
            token->lexeme = str;
            token->lexeme_len = 1;
            token->symbol = symbol;
            token->row = row;
            token->type = TT_SP_COMMA;
            tokens[count++] = token;
        }

        if (isdigit(command[i])) {
            size_t start = i;
            while (char_isnumber(command[i])) i++;
            size_t len = i - start;
            char *str = strndup(command + start, len);
            if (!str_isnumber(str)) {
                *out_error = ERR_SQL_INV_NUM_LIT; 
                return NULL;
            }
            TokenType numberType = str_isint(str) ? TT_LIT_INTEGER : TT_LIT_REAL;
            Token *token = create_token(str, len, numberType, row, symbol);
            tokens[count++] = token;
        }
    }

    *out_count = count; 
    return tokens;
}


