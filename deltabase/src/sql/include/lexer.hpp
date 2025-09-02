#ifndef SQL_LEXER_HPP
#define SQL_LEXER_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

extern "C" {
#include "../../core/include/meta.h"
}

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

        SELECT,
        FROM,
        INSERT,
        INTO,
        VALUES,
        UPDATE,
        SET,
        DELETE,
        WHERE,
        CREATE,
        DROP,
        DATABASE,
        TABLE,
        STRING,
        INTEGER,
        REAL,
        CHAR,
        BOOL,
        _NULL,
        PRIMARY,
        KEY,
        NOT,
        AUTOINCREMENT,
        UNIQUE,
        SCHEMA
    };

    enum class SqlSymbol {
        UNKNOWN = 0,

        LPAREN,
        RPAREN,
        COMMA,
        PERIOD,
        SEMICOLON
    };

    enum class SqlOperator {
        EQ = 1,
        NEQ,

        GR,
        GRE,
        LT,
        LTE,

        AND,
        OR,
        NOT,

        PLUS,
        MINUS,
        MUL,
        DIV,

        ASSIGN
    };

    enum class SqlLiteral {
        UNKNOWN = 0,

        STRING,
        INTEGER,
        BOOL,
        CHAR,
        REAL
    };

    inline auto
    keywords_map() -> const std::unordered_map<std::string, SqlKeyword>& {
        static const std::unordered_map<std::string, SqlKeyword> dictionary = {
            {"select", SqlKeyword::SELECT},
            {"from", SqlKeyword::FROM},
            {"insert", SqlKeyword::INSERT},
            {"into", SqlKeyword::INTO},
            {"values", SqlKeyword::VALUES},
            {"update", SqlKeyword::UPDATE},
            {"set", SqlKeyword::SET},
            {"delete", SqlKeyword::DELETE},
            {"where", SqlKeyword::WHERE},
            {"create", SqlKeyword::CREATE},
            {"drop", SqlKeyword::DROP},
            {"database", SqlKeyword::DATABASE},
            {"table", SqlKeyword::TABLE},
            {"string", SqlKeyword::STRING},
            {"integer", SqlKeyword::INTEGER},
            {"real", SqlKeyword::REAL},
            {"char", SqlKeyword::CHAR},
            {"bool", SqlKeyword::BOOL},
            {"null", SqlKeyword::_NULL},
            {"primary", SqlKeyword::PRIMARY},
            {"key", SqlKeyword::KEY},
            {"not", SqlKeyword::NOT},
            {"autoincrement", SqlKeyword::AUTOINCREMENT},
            {"unique", SqlKeyword::UNIQUE},
            {"schema", SqlKeyword::SCHEMA}
        };

        return dictionary;
    }

    inline auto
    data_types_map() -> const std::unordered_map<std::string, SqlKeyword>& {
        static const std::unordered_map<std::string, SqlKeyword> types_map = {
            {"string", SqlKeyword::STRING},
            {"integer", SqlKeyword::INTEGER},
            {"real", SqlKeyword::REAL},
            {"char", SqlKeyword::CHAR},
            {"bool", SqlKeyword::BOOL},
            {"null", SqlKeyword::_NULL}};

        return types_map;
    }

    inline auto
    is_data_type_kw(const SqlKeyword& kw) -> const bool {
        const auto& types_map = data_types_map();
        for (const auto& [key, value] : types_map) {
            if (value == kw) {
                return true;
            }
        }
        return false;
    }

    inline auto
    constraints_map() -> const std::unordered_map<std::string, SqlKeyword>& {
        static const std::unordered_map<std::string, SqlKeyword> constraints_map = {
            {"not", SqlKeyword::NOT},
            {"null", SqlKeyword::_NULL},
            {"primary", SqlKeyword::PRIMARY},
            {"key", SqlKeyword::KEY},
            {"autoincrement", SqlKeyword::AUTOINCREMENT},
            {"unique", SqlKeyword::UNIQUE},
        };

        return constraints_map;
    }

    inline auto
    is_constraint_kw(const SqlKeyword& kw) -> bool {
        const auto& constraints = constraints_map();
        for (const auto& [key, value] : constraints) {
            if (value == kw) {
                return true;
            }
        }
        return false;
    }

    inline auto
    symbols_map() -> const std::unordered_map<std::string, SqlSymbol>& {
        static const std::unordered_map<std::string, SqlSymbol> dictionary = {
            {"(", SqlSymbol::LPAREN},
            {")", SqlSymbol::RPAREN},
            {",", SqlSymbol::COMMA},
            {".", SqlSymbol::PERIOD},
            {";", SqlSymbol::SEMICOLON}};
        return dictionary;
    }

    inline auto
    operators_map() -> const std::unordered_map<std::string, SqlOperator>& {
        static const std::unordered_map<std::string, SqlOperator> dictionary = {
            {"==", SqlOperator::EQ},
            {"!=", SqlOperator::NEQ},

            {">", SqlOperator::GR},
            {">=", SqlOperator::GRE},
            {"<", SqlOperator::LT},
            {"<=", SqlOperator::LTE},

            {"and", SqlOperator::AND},
            {"or", SqlOperator::OR},
            {"not", SqlOperator::NOT},

            {"+", SqlOperator::PLUS},
            {"-", SqlOperator::MINUS},
            {"*", SqlOperator::MUL},
            {"/", SqlOperator::DIV},

            {"=", SqlOperator::ASSIGN},
        };
        return dictionary;
    }

    using SqlTokenDetail =
        std::variant<std::monostate, SqlKeyword, SqlOperator, SqlSymbol, SqlLiteral>;

    struct SqlToken {
        SqlTokenType type;
        std::string value;
        size_t line;
        size_t pos;
        SqlTokenDetail detail;

        SqlToken() = default;
        SqlToken(SqlTokenType type,
                 std::string value,
                 size_t line,
                 size_t position,
                 SqlTokenDetail detail = std::monostate());

        [[nodiscard]] auto
        is_data_type() const -> bool;
        [[nodiscard]] auto
        is_constraint() const -> bool;
        [[nodiscard]] auto
        is_keyword() const -> bool;

        template <typename TDetail>
        auto
        get_detail() const -> TDetail;

        [[nodiscard]] auto
        to_string(int indent = 4) const -> std::string;

        operator std::string() {
            return this->value;
        }
    };

    // Template definition must be in header
    template <typename TDetail>
    auto
    SqlToken::get_detail() const -> TDetail {
        if constexpr (std::is_same_v<TDetail, SqlKeyword> || std::is_same_v<TDetail, SqlOperator> ||
                      std::is_same_v<TDetail, SqlSymbol> || std::is_same_v<TDetail, SqlLiteral>) {
            if (std::holds_alternative<TDetail>(detail)) {
                return std::get<TDetail>(detail);
            }
            throw std::runtime_error("Invalid detail type requested");
        } else {
            static_assert(false, "Unsupported type for SqlTokenDetail");
        }
    }

    class SqlTokenizer {
      public:
        auto
        tokenize(const std::string& raw_sql) -> std::vector<SqlToken>;
    };

    namespace utils {
        auto
        to_string(sql::SqlTokenType type) -> std::string;
        auto
        to_string(sql::SqlKeyword type) -> std::string;
        auto
        to_string(sql::SqlOperator type) -> std::string;
        auto
        to_string(sql::SqlSymbol type) -> std::string;
        auto
        to_string(sql::SqlLiteral type, const std::string& value) -> std::string;

        auto
        get_data_type_str(DataType dt) -> std::string;

        } // namespace utils
} // namespace sql

#endif
