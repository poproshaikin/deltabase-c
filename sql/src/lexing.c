#include "sql-lexing.h"
#include "sql-token.h"
#include "sql-constants.h"
#include <ctype.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include "../../errors.h"
#include "../../utils/utils.h"

#define PARSING_FUNC_PARAMS const char *command, size_t *i, size_t len, size_t row, size_t *symbol, Error *out_error
#define PARSING_FUNC_ARGS command, &i, len, row, &symbol, out_error

const char space_delim = ' ';

void free_tokens(Token **tokens, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

void *handle_error(Token **tokens, size_t count) {
    printf("Failed to parse %lu element in lex()\n", count + 1);
    free_tokens(tokens, count);
    return NULL;
}

void prepare_command(char *cmd) {
    int len = strlen(cmd);
    if (str_contains(cmd, "\n")) {
        int index = 0;
        while ((index = str_indexat(cmd, '\n')) != -1) {
            str_rm(cmd, index);
        }   
    }
}

Token *parse_identifier(PARSING_FUNC_PARAMS) {
    size_t start = ++(*i);
    while (*i < len && command[*i] != '"') (*i)++;
    size_t tok_len = *i - start;
    char *str = strndup(command + start, tok_len);

    *symbol = *i;
    Token *token = create_token(str, tok_len, TT_IDENTIFIER, row, *symbol, out_error);

    return token;
}

Token *parse_string_literal(PARSING_FUNC_PARAMS) {
    size_t start = ++(*i);

    while (*i < len && command[*i] != '\'') i++;
    size_t tok_len = *i - start;
    char *str = strndup(command + start, tok_len);

    *symbol = *i;
    Token *token = create_token(str, tok_len, TT_LIT_STRING, row, *symbol, out_error);

    return token;
}

Token *parse_left_brace(size_t row, size_t symbol, Error *out_error) {
    char *str = malloc(2 * sizeof(char));
    str[0] = '(';
    str[1] = '\0';
    Token *token = create_token(str, 1, TT_SP_LEFT_BRACE, row, symbol, out_error);
    return token;
}

Token *parse_right_brace(size_t row, size_t symbol, Error *out_error) {
    char *str = malloc(2 * sizeof(char));
    str[0] = ')';
    str[1] = '\0';
    Token *token = create_token(str, 1, TT_SP_RIGHT_BRACE, row, symbol, out_error);
    return token;
}

Token *parse_comma(size_t row, size_t symbol, Error *out_error) {
    char *str = malloc(2 * sizeof(char));
    str[0] = ',';
    str[1] = '\0';
    Token *token = create_token(str, 1, TT_SP_COMMA, row, symbol, out_error);
    return token;
}

Token *parse_number_literal(PARSING_FUNC_PARAMS) {
    size_t start = *i;
    while (*i < len && char_isnumber(command[*i])) (*i)++;
    size_t tok_len = *i - start;
    char *str = strndup(command + start, tok_len);
    if (!str_isnumber(str)) {
        if (out_error) 
            *out_error = create_error(row, *symbol, ERR_SQL_INV_NUM_LIT);
        return NULL;
    }
    TokenType numberType = str_isint(str) ? TT_LIT_INTEGER : TT_LIT_REAL;
    Token *token = create_token(str, len, numberType, row, *symbol, out_error);
    return token;
}

Token *parse_keyword(PARSING_FUNC_PARAMS) {
    size_t start = *i;
    while (*i < len && isalnum(command[*i])) (*i)++;
    size_t tok_len = *i - start;
    char *str = str_tolower(strndup(command + start, tok_len));
    Token *token = NULL;
    if (is_operator(str)) {
        token = resolve_operator(str, row, symbol, out_error);
    }
    else {
        token = resolve_keyword(str, row, *symbol, out_error);
    }
    return token;
}

Token *parse_operator(PARSING_FUNC_PARAMS) {
    size_t start = *i;
    while (*i < len && !isspace(command[*i])) (*i)++;
    size_t tok_len = *i - start;
    char *str = strndup(command + start, tok_len);
    return resolve_operator(str, row, symbol, out_error);
}

Token **lex(char *command, size_t *out_count, Error *out_error) {
    size_t capacity = 128;
    size_t count = 0;
    size_t i = 0;
    size_t len = strlen(command);
    size_t row = 0, symbol = 0;

    Token **tokens = malloc(sizeof(Token *) * capacity);
    Token *token = NULL;

    prepare_command(command);

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

        if (command[i] == ';') {
            token = create_token(";", 1, TT_SP_TERMINATOR, row, symbol, out_error);
            i++;
        }

        else if (command[i] == '"') {
            token = parse_identifier(PARSING_FUNC_ARGS);
        }

        else if (command[i] == '\'') {
            token = parse_string_literal(PARSING_FUNC_ARGS);
        }

        else if (command[i] == '(') {
            token = parse_left_brace(row, symbol, out_error);
        }

        else if (command[i] == ')') {
            token = parse_right_brace(row, symbol, out_error);
        }

        else if (command[i] == ',') {
            token = parse_comma(row, symbol, out_error);
        }

        else if (isdigit(command[i])) {
            token = parse_number_literal(PARSING_FUNC_ARGS);
        }

        else if (isalpha(command[i])) {
            token = parse_keyword(PARSING_FUNC_ARGS);
        }

        else if (strchr("*=<>+-/", command[i])) {
            token = parse_operator(PARSING_FUNC_ARGS);
        }

        else {
            *out_error = create_error(row, symbol, ERR_SQL_UNEXP_SYM);
        }

        if (!token) {
            return handle_error(tokens, count);
        }

        tokens[count++] = token;
    }

    if (out_count)
        *out_count = count; 

    return tokens;
}
