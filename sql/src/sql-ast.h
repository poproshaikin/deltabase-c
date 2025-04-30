#ifndef SQL_AST_H
#define SQL_AST_H

#include "../../data/data-storage.h"
#include "sql-value.h"

typedef enum {
    AST_IDENTIFIER = 1,
    AST_LITERAL,
    AST_BINARY_EXPR,
    AST_SELECT,
    AST_INSERT,
    AST_UPDATE,
    AST_DELETE
} AstNodeType;

typedef struct AstLiteral {
    ValueType type;
    union {
        struct { 
            char *str_value; 
            dulen_t len;
        } string; 
        int int_value;
        double real_value;
        bool bool_value;
    };
} AstLiteral;

typedef struct AstIdentifier {
    char *name;
    dulen_t len;
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
