#ifndef SQL_AST_H
#define SQL_AST_H

#include "../../data/data-storage.h"
#include "sql-value.h"

typedef enum {
    AST_N_IDENTIFIER = 1,
    AST_N_LITERAL,
    AST_N_BINARY_EXPR,
    AST_N_SELECT,
    AST_N_INSERT,
    AST_N_UPDATE,
    AST_N_DELETE
} AstNodeType;

typedef struct AstLiteral {
    Token *token;
} AstLiteral;

typedef struct AstIdentifier {
    Token *token;
} AstIdentifier;

typedef enum AstOperatorType {
    ASTOP_ADD = 1,
    ASTOP_SUBTRACTION,
    ASTOP_MULTIPLICATION,
    ASTOP_DIVISION,
    ASTOP_AND,
    ASTOP_OR
} AstOperatorType;

typedef struct AstNode AstNode; // forward declaration

struct AstNode {
    AstNodeType type;
    union {
        AstLiteral literal;
        AstIdentifier identifier;

        struct {
            AstNode *table;
            int columns_count;
            AstNode **columns;
            AstNode *where;
        } select;

        struct {
            AstNode *table;
            int columns_count;
            AstNode **columns;
            AstNode **values;
        } insert;

        struct {
            AstNode *left;
            AstNode *right;
            AstOperatorType operation;           
        } binary_expr;
    } value;
};


#endif
