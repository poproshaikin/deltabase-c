#ifndef SQL_ENGINE_H
#define SQL_ENGINE_H

#include "src/sql-value.h"
#include "src/sql-ast.h"

Value eval(AstNode *node);

#endif
