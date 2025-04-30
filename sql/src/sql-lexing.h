#ifndef SQL_LEXING_H
#define SQL_LEXING_H

#include "sql-ast.h"

Token *lex(const char *command, int len);
 
#endif
