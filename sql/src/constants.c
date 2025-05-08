#include "sql-constants.h"
#include <string.h>

Token *resolve_keyword(char *str, size_t row, size_t symbol, Error *out_error) {
    size_t keywordsCount = sizeof(keywords) / sizeof(Keyword);
    for (size_t i = 0; i < keywordsCount; i++) {
        const Keyword *kw = &keywords[i]; 
        if (strcmp(str, kw->lexeme) == 0) {
            return create_token(str, strlen(str), kw->tokenType, row, symbol, out_error);
        }
    }
    
    if (out_error) {
        *out_error = create_error(row, symbol, ERR_SQL_INV_STX_KW);
    }
    return NULL;
}

bool is_operator(char *str) {
    size_t operatorsCount = sizeof(operators) / sizeof(Operator);
    for (size_t i = 0; i < operatorsCount; i++) {
        const Operator *op = &operators[i];
        if (strcmp(str, op->lexeme) == 0) {
            return true;
        }
    }
    return false;
}

Token *resolve_operator(char *str, size_t row, size_t *symbol, Error *out_error) {
    size_t operatorsCount = sizeof(operators) / sizeof(Operator);
    for (size_t i = 0; i < operatorsCount; i++) {
        const Operator *op = &operators[i];
        if (strcmp(str, op->lexeme) == 0) {
            return create_token(str, strlen(str), op->tokenType, row, *symbol, out_error);
        }
    }

    if (out_error) {
        *out_error = create_error(row, *symbol, ERR_SQL_INV_STX_KW);
    }
    return NULL;
}
