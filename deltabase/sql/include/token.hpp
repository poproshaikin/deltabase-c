#pragma once

#include <string>
#include <variant>

namespace sql {
    enum SqlTokenType {
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
    };

    using SqlTokenValue = std::variant<int, double, std::string, bool>;

    struct SqlToken {
        std::string lexeme;
        SqlTokenType type;
        SqlTokenValue value;
        
        SqlToken(std::string lexeme, SqlTokenType type, SqlTokenValue value);
    };

}
