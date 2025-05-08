#ifndef SQL_CONSTANTS_H
#define SQL_CONSTANTS_H

#include "../../utils/utils.h"
#include "../../errors.h"
#include "sql-token.h"

typedef struct Keyword {
    const char *lexeme;
    const TokenType tokenType;
} Keyword;
  
static const Keyword keywords[] = {
    { "select",   TT_KW_SELECT },
    { "from",     TT_KW_FROM },
    { "where",    TT_KW_WHERE },
    { "insert",   TT_KW_INSERT },
    { "into",     TT_KW_INTO },
    { "values",   TT_KW_VALUES },
};

Token *resolve_keyword(char *str, size_t row, size_t symbol, Error *out_error);

typedef struct Operator {
    const char *lexeme;
    const TokenType tokenType;
} Operator;

static const Operator operators[] = {
    { "==",  TT_OP_EQUAL },
    { "<",   TT_OP_LESS },
    { ">",   TT_OP_GREATER },
    { "AND", TT_OP_AND },
    { "OR",  TT_OP_OR },
    { "*",   TT_OP_ASTERISK },
};

bool is_operator(char *str);
Token *resolve_operator(char *str, size_t row, size_t *symbol, Error *out_error);

#endif
