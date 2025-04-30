#ifndef SQL_VALUE_H
#define SQL_VALUE_H

#include "../../data/data-storage.h"

typedef enum TokenType {
    TT_INTEGER = 1,
    TT_REAL,
    TT_STRING,
    TT_BOOL,
    TT_TABLE,
} TokenType;

typedef struct TableColumn {
    char *name;
    int name_len;
    TokenType tokenType;
} TableColumn;

typedef struct TableRow TableRow; // forward declaration

typedef struct Table {
    TableColumn *columns;
    int columns_count;

    TableRow *rows;
    int rows_count;
} Table;

typedef struct Token {
    TokenType type;
    char *lexeme;
    dulen_t lexeme_len;
    union {
        int int_value;
        double real_value;
        struct {
            char *value;
            dulen_t len;
        } string;
        bool bool_value;
        Table *table; 
    } value;
} Token;

struct TableRow {
    Token *tokens;
    dulen_t count;
};

#endif
