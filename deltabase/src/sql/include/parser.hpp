//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_PARSER_HPP
#define DELTABASE_PARSER_HPP

#include "../../types/include/sql_token.hpp"
#include "../../types/include/ast_tree.hpp"

#include <vector>


namespace sql
{
    class SqlParser
    {
        std::vector<types::SqlToken> tokens_;
        size_t current_;

        types::SelectStatement
        parse_select();

        types::InsertStatement
        parse_insert();

        types::UpdateStatement
        parse_update();

        types::DeleteStatement
        parse_delete();

        types::CreateTableStatement
        parse_create_table();

        types::CreateDbStatement
        parse_create_db();

        types::CreateSchemaStatement
        parse_create_schema();

        types::BinaryExpr
        parse_binary(int min_priority);

        template <typename TEnum>
        bool
        match(const TEnum& expected) const
        {
            if (current_ >= tokens_.size())
                return false;

            if constexpr (std::is_same_v<TEnum, types::SqlTokenType>)
                return tokens_[current_].type == expected;

            else if constexpr (std::is_same_v<TEnum, std::monostate> ||
                               std::is_same_v<TEnum, types::SqlOperator> ||
                               std::is_same_v<TEnum, types::SqlSymbol> ||
                               std::is_same_v<TEnum, types::SqlKeyword> ||
                               std::is_same_v<TEnum, types::SqlLiteral>)
            {
                const auto* value = std::get_if<TEnum>(&tokens_[current_].detail);
                return value && *value == expected;
            }
            else
                throw std::runtime_error("Unsupported type passed for match()");
        }

        template <typename TEnum>
        void
        match_or_throw(TEnum expected, std::string error_msg = "Invalid syntax") const
        {
            if (!match<TEnum>(expected))
                throw std::runtime_error(error_msg);
        }

        bool
        advance() noexcept;

        bool
        advance_or_throw(const std::string& error_msg = "Invalid statement syntax");

        const types::SqlToken&
        previous() const noexcept;

        const types::SqlToken&
        current() const;

        std::unique_ptr<types::AstNode>
        parse_primary();

        std::vector<types::AstNode>
        parse_tokens_list(types::SqlTokenType tokenType, types::AstNodeType nodeType);

        types::ColumnDefinition
        parse_column_def();

        types::TableIdentifier
        parse_table_identifier();

    public:
        explicit
        SqlParser(std::vector<types::SqlToken> tokens);

        types::AstNode
        parse();
    };
}

#endif //DELTABASE_PARSER_HPP