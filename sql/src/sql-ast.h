#ifndef SQL_AST_H
#define SQL_AST_H

#include "../../data/data-storage.h"
#include "sql-token.h"

typedef enum {
    NT_IDENTIFIER = 1,
    NT_LITERAL,
    NT_BINARY_EXPR,
    NT_SELECT,
    NT_INSERT,
    NT_UPDATE,
    NT_DELETE
} AstNodeType;

typedef struct AstLiteral {
    Token *token;
} AstLiteral;

typedef struct AstIdentifier {
    Token *token;
} AstIdentifier;

typedef enum AstOperatorType {
    OP_ADD = 1, // addition
    OP_SUB,     // subtraction
    OP_MUL,     // multiplication
    OP_DIV,     // division

    OP_AND,     // binary AND
    OP_OR,      // binary OR

    OP_EQU,     // equality
    OP_GRT,     // greater than
    OP_LST      // less than
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
