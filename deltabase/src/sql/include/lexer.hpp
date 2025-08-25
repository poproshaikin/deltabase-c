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
        PRIMARY, KEY, NOT, AUTOINCREMENT, UNIQUE
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

    inline const std::unordered_map<std::string, SqlKeyword>& keywords_map() {
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
            {"autoincrement", SqlKeyword::AUTOINCREMENT },
            {"unique",        SqlKeyword::UNIQUE }
        };

        return dictionary;
    }

    inline const std::unordered_map<std::string, SqlKeyword>& data_types_map() {
        static const std::unordered_map<std::string, SqlKeyword> types_map = {
            { "string",  SqlKeyword::STRING },
            { "integer", SqlKeyword::INTEGER },
            { "real",    SqlKeyword::REAL },
            { "char",    SqlKeyword::CHAR },
            { "bool",    SqlKeyword::BOOL },
            { "null",    SqlKeyword::_NULL }
        };
        
        return types_map;
    }

    inline const bool is_data_type_kw(const SqlKeyword& kw) {
        const auto& types_map = data_types_map();
        for (const auto& [key, value] : types_map) {
            if (value == kw) {
                return true;
            }
        }
        return false;
    }

    inline const std::unordered_map<std::string, SqlKeyword>& constraints_map() {
        static const std::unordered_map<std::string, SqlKeyword> constraints_map = {
            { "not",          SqlKeyword::NOT },
            { "null",         SqlKeyword::_NULL },
            { "primary",      SqlKeyword::PRIMARY },
            { "key",          SqlKeyword::KEY },
            {"autoincrement", SqlKeyword::AUTOINCREMENT },
            {"unique",        SqlKeyword::UNIQUE },
        };

        return constraints_map;
    }

    inline bool is_constraint_kw(const SqlKeyword& kw) {
        const auto& constraints = constraints_map();
        for (const auto& [key, value] : constraints) {
            if (value == kw) {
                return true;
            }
        }
        return false;
    }

    inline const std::unordered_map<std::string, SqlSymbol>& symbols_map() {
        static const std::unordered_map<std::string, SqlSymbol> dictionary = {
            { "(", SqlSymbol::LPAREN },
            { ")", SqlSymbol::RPAREN },
            { ",", SqlSymbol::COMMA },
            { ".", SqlSymbol::POINT },
            { ";", SqlSymbol::SEMICOLON }
        };
        return dictionary;
    }

    inline const std::unordered_map<std::string, SqlOperator>& operators_map() {
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

        bool is_data_type() const;
        bool is_constraint() const;
        bool is_keyword() const;

        template<typename TDetail>
        TDetail get_detail() const;

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
