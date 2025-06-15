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

        SELECT, FROM, INSERT, INTO, UPDATE, SET, DELETE, WHERE,
        CREATE, DROP, DATABASE, TABLE,
        STRING, INTEGER, REAL, CHAR, BOOL, _NULL,
    };

    enum class SqlSymbol {
        UNKNOWN = 0,

        LPAREN, RPAREN,
        COMMA, POINT, SEMICOLON
    };

    enum class SqlOperator {
        EQ = 1,
        NEQ,
        
        GR,
        GRT,
        LT,
        LTE,

        AND, OR,

        PLUS, MINUS, MUL, DIV,

        ASSIGN
    };

    enum class SqlLiteral {
        UNKNOWN = 0,

        STRING, INTEGER, BOOL, CHAR, REAL
    };

    inline const std::unordered_map<std::string, SqlKeyword>& getKeywordsMap() {
        static const std::unordered_map<std::string, SqlKeyword> dictionary = {
            { "SELECT",   SqlKeyword::SELECT },
            { "FROM",     SqlKeyword::FROM },
            { "INSERT",   SqlKeyword::INSERT },
            { "INTO",     SqlKeyword::INTO },
            { "UPDATE",   SqlKeyword::UPDATE },
            { "SET",      SqlKeyword::SET },
            { "DELETE",   SqlKeyword::DELETE }, 
            { "WHERE",    SqlKeyword::WHERE },
            { "CREATE",   SqlKeyword::CREATE },
            { "DROP",     SqlKeyword::DROP },
            { "DATABASE", SqlKeyword::DATABASE },
            { "TABLE",    SqlKeyword::TABLE },
            { "STRING",   SqlKeyword::STRING },
            { "INTEGER",  SqlKeyword::INTEGER },
            { "REAL",     SqlKeyword::REAL },
            { "CHAR",     SqlKeyword::CHAR },
            { "BOOL",     SqlKeyword::BOOL },
            { "NULL",     SqlKeyword::_NULL },
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
            { ">=", SqlOperator::GRT },
            { "<",  SqlOperator::LT },
            { "<=", SqlOperator::LTE },

            { "AND", SqlOperator::AND },
            { "OR", SqlOperator::OR },

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

        SqlToken(SqlTokenType type, std::string value, size_t line, size_t position, SqlTokenDetail detail = std::monostate());

        std::string to_string() const;
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
