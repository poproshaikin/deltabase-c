#ifndef SQL_TOKEN_H
#define SQL_TOKEN_H

#include "../../data/data-storage.h"
#include "../../errors.h"

typedef enum TokenType {
    TT_UNDEFINED = 0,

    TT_IDENTIFIER = 1,
    TT_TABLE_IDENTIFIER,
    TT_COLUMN_IDENTIFIER,

    TT_LIT_STRING = 10,
    TT_LIT_INTEGER,
    TT_LIT_REAL,
    TT_LIT_BOOLEAN,
    TT_LIT_CHAR,

    TT_KW_SELECT = 20,
    TT_KW_FROM,
    TT_KW_WHERE,
    TT_KW_INSERT,
    TT_KW_INTO,
    TT_KW_VALUES,

    TT_OP_EQUAL = 30,
    TT_OP_LESS,
    TT_OP_GREATER,
    TT_OP_AND,
    TT_OP_OR,
    TT_OP_ASSIGN,
    TT_OP_ASTERISK,

    TT_SP_LEFT_BRACE = 40,
    TT_SP_RIGHT_BRACE,
    TT_SP_COMMA,
    TT_SP_TERMINATOR
} TokenType;

typedef struct Token {
    TokenType type;
    char *lexeme;
    size_t lexeme_len;
    union {
        int int_value;
        double real_value;
        struct {
            char *value;
            size_t len;
        } string;
        bool bool_value;
    } value;

    int symbol; // for describing errors
    int row;    // for describing errors
} Token;

Token *create_token(char *lexeme, 
    size_t lexeme_len, 
    TokenType type, 
    size_t row, 
    size_t symbol,
    Error *out_error
);

typedef struct TableRow {
    Token *tokens;
    size_t count;
} TableRow;

typedef struct TableColumn {
    char *name;
    int name_len;
    TokenType tokenType;
} TableColumn;

typedef struct Table {
    TableColumn *columns;
    int columns_count;

    TableRow *rows;
    int rows_count;
} Table;

#endif
