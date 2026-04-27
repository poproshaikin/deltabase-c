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
        SCHEMA,
        INDEX,
        ON,
        IS,
        ALTER,
        ADD,
        COLUMN
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

        IS,

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
        INTEGER = 1,
        STRING,
        BOOL,
        CHAR,
        REAL,
        NULL_,

        COUNT
    };

    using SqlTokenDetail =
        std::variant<std::monostate, SqlKeyword, SqlOperator, SqlSymbol, SqlLiteral>;

    struct SqlToken
    {
        SqlTokenType type;
        std::string value;
        size_t line;
        size_t pos;
        SqlTokenDetail detail;

        SqlToken() = default;

        SqlToken(
            SqlTokenType type,
            std::string value,
            size_t line,
            size_t position,
            SqlTokenDetail detail = std::monostate()
        );

        bool
        is_data_type() const;

        bool
        is_constraint() const;

        bool
        is_keyword() const;

        bool
        is_literal() const;

        bool
        is_identifier() const;

        template <typename TDetail>
        TDetail
        get_detail() const
        {
            if constexpr (std::is_same_v<TDetail, SqlKeyword> ||
                          std::is_same_v<TDetail, SqlOperator> ||
                          std::is_same_v<TDetail, SqlSymbol> || std::is_same_v<TDetail, SqlLiteral>)
            {
                if (std::holds_alternative<TDetail>(detail))
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
} // namespace types

#endif // DELTABASE_SQL_TOKEN_HPP