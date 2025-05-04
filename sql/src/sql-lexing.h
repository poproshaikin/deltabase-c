#ifndef SQL_LEXING_H
#define SQL_LEXING_H

#include "sql-ast.h"

#include "../../errors.h"

Token **lex(char *command, size_t *out_count, ErrorCode *out_error);
 
#endif
