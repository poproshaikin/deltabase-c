#ifndef SQL_CONSTANTS_H
#define SQL_CONSTANTS_H

#include "../../utils/utils.h"
#include "sql-token.h"

typedef struct Keyword {
    const char *lexeme;
    const TokenType tokenType;
} Keyword;
  
const Keyword keywords[] = {
    { "SELECT",   TT_KW_SELECT },
    { "FROM",     TT_KW_FROM },
    { "WHERE",    TT_KW_WHERE },
    { "INSERT",   TT_KW_INSERT },
    { "INTO",     TT_KW_INTO },
    { "VALUES",   TT_KW_VALUES },
};

typedef struct Operator {
    const char *lexeme;
    const TokenType tokenType;
} Operator;

const Operator operators[] = {
    { "==",  TT_OP_EQUAL },
    { "<",   TT_OP_LESS },
    { ">",   TT_OP_GREATER },
    { "AND", TT_OP_AND },
    { "OR",  TT_OP_OR },
};

void init_sql_constants();

#endif
