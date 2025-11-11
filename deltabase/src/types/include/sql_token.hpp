//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_SQL_TOKEN_HPP
#define DELTABASE_SQL_TOKEN_HPP

#include <stdexcept>
#include <unordered_map>
#include <variant>

namespace types
{
    enum class SqlTokenType
    {
        UNKNOWN = 0,

        KEYWORD = 1,
        IDENTIFIER,
        LITERAL,
        OPERATOR,
        SYMBOL,
    };

    enum class SqlKeyword
    {
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

    enum class SqlSymbol
    {
        UNKNOWN = 0,

        LPAREN,
        RPAREN,
        COMMA,
        PERIOD,
        SEMICOLON
    };

    enum class SqlOperator
    {
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

    enum class SqlLiteral
    {
        UNKNOWN = 0,

        STRING,
        INTEGER,
        BOOL,
        CHAR,
        REAL
    };

    inline auto
    keywords_map() -> const std::unordered_map<std::string, SqlKeyword>&
    {
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
    data_types_map() -> const std::unordered_map<std::string, SqlKeyword>&
    {
        static const std::unordered_map<std::string, SqlKeyword> types_map = {
            {"string", SqlKeyword::STRING},
            {"integer", SqlKeyword::INTEGER},
            {"real", SqlKeyword::REAL},
            {"char", SqlKeyword::CHAR},
            {"bool", SqlKeyword::BOOL},
            {"null", SqlKeyword::_NULL}};

        return types_map;
    }

    inline bool
    is_data_type_kw(const SqlKeyword& kw)
    {
        const auto& types_map = data_types_map();
        for (const auto& [key, value] : types_map)
        {
            if (value == kw)
            {
                return true;
            }
        }
        return false;
    }

    inline const std::unordered_map<std::string, SqlKeyword>&
    constraints_map()
    {
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

    inline bool
    is_constraint_kw(const SqlKeyword& kw)
    {
        const auto& constraints = constraints_map();
        for (const auto& [key, value] : constraints)
        {
            if (value == kw)
            {
                return true;
            }
        }
        return false;
    }

    inline const std::unordered_map<std::string, SqlSymbol>&
    symbols_map()
    {
        static const std::unordered_map<std::string, SqlSymbol> dictionary = {
            {"(", SqlSymbol::LPAREN},
            {")", SqlSymbol::RPAREN},
            {",", SqlSymbol::COMMA},
            {".", SqlSymbol::PERIOD},
            {";", SqlSymbol::SEMICOLON}};
        return dictionary;
    }

    inline const std::unordered_map<std::string, SqlOperator>&
    operators_map()
    {
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

    using SqlTokenDetail = std::variant<std::monostate, SqlKeyword, SqlOperator, SqlSymbol,
        SqlLiteral>;

    struct SqlToken
    {
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

        bool
        is_data_type() const;

        bool
        is_constraint() const;

        bool
        is_keyword() const;

        template <typename TDetail>
        TDetail
        get_detail() const
        {
            if constexpr (
                std::is_same_v<TDetail, SqlKeyword> || std::is_same_v<TDetail, SqlOperator> ||
                std::is_same_v<TDetail, SqlSymbol> || std::is_same_v<TDetail, SqlLiteral>)
            {
                if constexpr (std::holds_alternative<TDetail>(detail))
                {
                    return std::get<TDetail>(detail);
                }
                throw std::runtime_error("Invalid detail type requested");
            }
            else
            {
                static_assert(false, "Unsupported type for SqlTokenDetail");
                throw std::runtime_error("Unsupported type for SqlToken");
            }
        }

        std::string
        to_string(int indent = 4) const;

        operator std::string()
        {
            return this->value;
        }
    };
}

#endif //DELTABASE_SQL_TOKEN_HPP