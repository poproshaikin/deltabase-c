#ifndef SQL_LEXER_HPP
#define SQL_LEXER_HPP

#include <cstddef>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace sql {
    enum class SqlTokenType {
        UNKNOWN = 0,

        KEYWORD = 1,
        IDENTIFIER,
        LITERAL,
        OPERATOR,
        SYMBOL,
    };

    enum class SqlKeyword {
        UNKNOWN = 0,

        SELECT, FROM, INSERT, INTO, VALUES, UPDATE, SET, DELETE, WHERE,
        CREATE, DROP, DATABASE, TABLE,
        STRING, INTEGER, REAL, CHAR, BOOL, _NULL,
        PRIMARY, KEY, NOT, AUTOINCREMENT
    };

    enum class SqlSymbol {
        UNKNOWN = 0,

        LPAREN, RPAREN,
        COMMA, POINT, SEMICOLON
    };

    enum class SqlOperator {
        EQ = 1,
        NEQ,
        
        GR, GRE,
        LT, LTE,

        AND, OR, NOT,

        PLUS, MINUS, MUL, DIV,

        ASSIGN
    };

    enum class SqlLiteral {
        UNKNOWN = 0,

        STRING, INTEGER, BOOL, CHAR, REAL
    };

    inline const std::unordered_map<std::string, SqlKeyword>& getKeywordsMap() {
        static const std::unordered_map<std::string, SqlKeyword> dictionary = {
            { "select",       SqlKeyword::SELECT },
            { "from",         SqlKeyword::FROM },
            { "insert",       SqlKeyword::INSERT },
            { "into",         SqlKeyword::INTO },
            { "values",       SqlKeyword::VALUES },
            { "update",       SqlKeyword::UPDATE },
            { "set",          SqlKeyword::SET },
            { "delete",       SqlKeyword::DELETE }, 
            { "where",        SqlKeyword::WHERE },
            { "create",       SqlKeyword::CREATE },
            { "drop",         SqlKeyword::DROP },
            { "database",     SqlKeyword::DATABASE },
            { "table",        SqlKeyword::TABLE },
            { "string",       SqlKeyword::STRING },
            { "integer",      SqlKeyword::INTEGER },
            { "real",         SqlKeyword::REAL },
            { "char",         SqlKeyword::CHAR },
            { "bool",         SqlKeyword::BOOL },
            { "null",         SqlKeyword::_NULL },
            {"primary",       SqlKeyword::PRIMARY},
            {"key",           SqlKeyword::KEY},
            {"not",           SqlKeyword::NOT },
            {"autoincrement", SqlKeyword::AUTOINCREMENT }
        };

        return dictionary;
    }

    inline const std::unordered_map<std::string, SqlSymbol>& getSymbolsMap() {
        static const std::unordered_map<std::string, SqlSymbol> dictionary = {
            { "(", SqlSymbol::LPAREN },
            { ")", SqlSymbol::RPAREN },
            { ",", SqlSymbol::COMMA },
            { ".", SqlSymbol::POINT },
            { ";", SqlSymbol::SEMICOLON }
        };
        return dictionary;
    }

    inline const std::unordered_map<std::string, SqlOperator>& getOperatorsMap() {
        static const std::unordered_map<std::string, SqlOperator> dictionary = {
            { "==",  SqlOperator::EQ },
            { "!=", SqlOperator::NEQ },

            { ">",  SqlOperator::GR },
            { ">=", SqlOperator::GRE },
            { "<",  SqlOperator::LT },
            { "<=", SqlOperator::LTE },

            { "and", SqlOperator::AND },
            { "or",  SqlOperator::OR  },
            { "not", SqlOperator::NOT },

            { "+",  SqlOperator::PLUS },
            { "-",  SqlOperator::MINUS },
            { "*",  SqlOperator::MUL },
            { "/",  SqlOperator::DIV },

            { "=", SqlOperator::ASSIGN },
        };
        return dictionary;
    }

    using SqlTokenDetail = std::variant<std::monostate, SqlKeyword, SqlOperator, SqlSymbol, SqlLiteral>;

    struct SqlToken {
        SqlTokenType type;
        std::string value;
        size_t line;
        size_t pos;
        SqlTokenDetail detail;

        SqlToken() = default;
        SqlToken(SqlTokenType type, std::string value, size_t line, size_t position, SqlTokenDetail detail = std::monostate());

        std::string to_string(int indent = 4) const;
    };

    class SqlTokenizer {
        public:
            std::vector<SqlToken> tokenize(const std::string& raw_sql);
    };

    namespace utils {
        std::string to_string(sql::SqlTokenType type);
        std::string to_string(sql::SqlKeyword type);
        std::string to_string(sql::SqlOperator type);
        std::string to_string(sql::SqlSymbol type);
        std::string to_string(sql::SqlLiteral type, const std::string& value);
    }
}

#endif
