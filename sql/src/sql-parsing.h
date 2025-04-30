#ifndef SQL_PARSING_H
#define SQL_PARSING_H

#include "sql-ast.h"

AstNode *parse(char *command, int len);

#endif
