#include "include/parser.hpp"
#include "../misc/include/exceptions.hpp"
#include "include/lexer.hpp"
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>

namespace sql
{
    AstNode::AstNode(AstNodeType type, AstNodeValue&& value) : type(type), value(std::move(value))
    {
    }

    SqlParser::SqlParser(std::vector<SqlToken> tokens) : tokens_(std::move(tokens)), current_(0)
    {
    }

    auto
    SqlParser::advance() noexcept -> bool
    {
        if (current_ + 1 < tokens_.size())
        {
            current_++;
            return true;
        }

        return false;
    }

    auto
    SqlParser::advance_or_throw(std::string error_msg) -> bool
    {
        if (!advance())
            throw std::runtime_error(error_msg);

        return true;
    }

    const SqlToken&
    SqlParser::previous() const noexcept
    {
        return tokens_[current_ - 1];
    }

    template <typename T, typename Variant> struct is_in_variant;

    template <typename T, typename... Ts> struct is_in_variant<T, std::variant<Ts...> >
    {
        static constexpr bool value = (std::is_same_v<T, Ts> || ...);
    };

    template <typename T, typename Variant>
    constexpr bool is_in_variant_v = is_in_variant<T, Variant>::value;

    template <typename TEnum>
    bool
    SqlParser::match(const TEnum& expected) const
    {
        if (current_ >= tokens_.size())
            return false;

        if constexpr (std::is_same_v<TEnum, SqlTokenType>)
            return tokens_[current_].type == expected;

        else if constexpr (std::is_same_v<TEnum, std::monostate> ||
                           std::is_same_v<TEnum, SqlOperator> ||
                           std::is_same_v<TEnum, SqlSymbol> ||
                           std::is_same_v<TEnum, SqlKeyword> ||
                           std::is_same_v<TEnum, SqlLiteral>)
        {
            const auto* value = std::get_if<TEnum>(&tokens_[current_].detail);
            return value && *value == expected;
        }
        else
            throw std::runtime_error("Unsupported type passed for match()");
    }

    template <typename TEnum>
    void
    SqlParser::match_or_throw(TEnum expected, const std::string error_msg) const
    {
        if (!match<TEnum>(expected))
            throw std::runtime_error(error_msg);
    }

    const SqlToken&
    SqlParser::current() const
    {
        if (current_ >= tokens_.size())
            throw std::out_of_range("Parser: current() out of bounds");

        return tokens_[current_];
    }

    AstNode
    SqlParser::parse()
    {
        if (match(SqlKeyword::SELECT))
        {
            return AstNode(AstNodeType::SELECT, parse_select());
        }
        if (match(SqlKeyword::INSERT))
        {
            return AstNode(AstNodeType::INSERT, parse_insert());
        }
        if (match(SqlKeyword::UPDATE))
        {
            return AstNode(AstNodeType::UPDATE, parse_update());
        }
        if (match(SqlKeyword::DELETE))
        {
            return AstNode(AstNodeType::DELETE, parse_delete());
        }
        if (match(SqlKeyword::CREATE))
        {
            if (!advance())
                throw InvalidStatementSyntax();

            if (match(SqlKeyword::TABLE))
                return AstNode(AstNodeType::CREATE_TABLE, parse_create_table());

            if (match(SqlKeyword::DATABASE))
                return AstNode(AstNodeType::CREATE_DATABASE, parse_create_db());

            if (match(SqlKeyword::SCHEMA))
                return AstNode(AstNodeType::CREATE_SCHEMA, parse_create_schema());
        }

        throw std::runtime_error("Unsupported statement");
    }

    SelectStatement
    SqlParser::parse_select()
    {
        SelectStatement stmt;

        if (!match(SqlOperator::MUL))
        {
            while (true)
            {
                advance_or_throw("Invalid statement syntax");
                if (!match(SqlTokenType::IDENTIFIER))
                    break;

                stmt.columns.push_back(current());

                if (!advance() || !match(SqlSymbol::COMMA))
                    break;
            }
        }
        else
            advance();

        advance_or_throw();
        match_or_throw(SqlKeyword::FROM, "Expected 'FROM'");

        advance_or_throw();
        stmt.table = parse_table_identifier();

        if (advance() && match(SqlKeyword::WHERE))
        {
            advance_or_throw();
            stmt.where = parse_binary(0);
        }

        return stmt;
    }

    InsertStatement
    SqlParser::parse_insert()
    {
        InsertStatement stmt;

        advance_or_throw();
        match_or_throw(SqlKeyword::INTO, "Expected 'INTO' keyword");

        advance_or_throw();
        stmt.table = parse_table_identifier();

        match_or_throw(SqlSymbol::LPAREN, "Expected left parenthesis");

        if (!match(SqlOperator::MUL))
        {
            while (true)
            {
                advance_or_throw("Invalid statement syntax");
                if (!match(SqlTokenType::IDENTIFIER))
                {
                    break;
                }
                stmt.columns.push_back(current());

                if (!advance() || !match(SqlSymbol::COMMA))
                {
                    current_--;
                    break;
                }
            }
        }
        else
        {
            advance_or_throw("Invalid statement syntax");
        }

        advance_or_throw();
        match_or_throw(SqlSymbol::RPAREN, "Expected right parenthesis");

        advance_or_throw();
        match_or_throw(SqlKeyword::VALUES, "Expected 'VALUES' keyword");

        advance_or_throw();
        match_or_throw(SqlSymbol::LPAREN, "Expected left parenthesis");

        while (true)
        {
            advance_or_throw("Invalid statement syntax");
            if (!match(SqlTokenType::LITERAL))
            {
                break;
            }
            stmt.values.push_back(current());

            if (!advance() || !match(SqlSymbol::COMMA))
            {
                current_--;
                break;
            }
        }

        advance_or_throw();
        match_or_throw(SqlSymbol::RPAREN, "Expected right parenthesis");

        return stmt;
    }

    auto
    SqlParser::parse_update() -> UpdateStatement
    {
        UpdateStatement stmt;

        advance_or_throw();
        stmt.table = parse_table_identifier();

        match_or_throw(SqlKeyword::SET, "Expected 'SET' keyword");

        while (true)
        {
            advance_or_throw();
            BinaryExpr expr = parse_binary(0);

            if (expr.op != AstOperator::ASSIGN)
                throw std::runtime_error("Expected assignment expression (col = value)");

            stmt.assignments.emplace_back(std::move(expr));

            if (!match(SqlSymbol::COMMA))
                break;
        }

        if (match(SqlKeyword::WHERE))
        {
            advance();
            stmt.where = parse_binary(0);
        }

        return stmt;
    }

    auto
    SqlParser::parse_delete() -> DeleteStatement
    {
        DeleteStatement stmt;

        advance_or_throw();
        match_or_throw(SqlKeyword::FROM, "Expected 'FROM' keyword after DELETE");

        advance_or_throw();
        stmt.table = parse_table_identifier();

        if (match(SqlKeyword::WHERE))
        {
            advance();
            stmt.where = parse_binary(0);
        }

        return stmt;
    }

    auto
    SqlParser::parse_create_table() -> CreateTableStatement
    {
        CreateTableStatement stmt;

        match_or_throw(SqlKeyword::TABLE, "Expected 'TABLE' after 'CREATE'");

        advance_or_throw();
        stmt.table = parse_table_identifier();

        if (match(SqlSymbol::LPAREN))
        {
            advance_or_throw();
            bool stop = false;
            while (!stop)
            {
                stmt.columns.push_back(parse_column_def());

                if (match(SqlSymbol::RPAREN))
                {
                    stop = true;
                }
                advance();
            }
        }

        return stmt;
    }

    auto
    SqlParser::parse_column_def() -> ColumnDefinition
    {
        ColumnDefinition def;

        match_or_throw(SqlTokenType::IDENTIFIER, "Expected column identifier");
        def.name = current();

        advance_or_throw();

        def.type = current();
        advance_or_throw();

        if (match(SqlSymbol::COMMA) || match(SqlSymbol::RPAREN))
        {
            return def;
        }

        advance_or_throw();

        while (true)
        {
            const SqlToken& cur = current();

            if (match(SqlSymbol::COMMA) || match(SqlSymbol::RPAREN))
                return def;

            if (!std::holds_alternative<SqlKeyword>(cur.detail))
                throw InvalidStatementSyntax();

            if (!cur.is_constraint() || !cur.is_data_type())
                throw InvalidStatementSyntax();

            SqlToken copy = cur;
            def.constraints.push_back(copy);
            advance_or_throw();
        }

        return def;
    }

    std::vector<std::unique_ptr<AstNode>>
    SqlParser::parse_tokens_list(SqlTokenType tokenType,
                                 AstNodeType nodeType)
    {
        std::vector<std::unique_ptr<AstNode>> tokens;

        while (true)
        {
            if (!advance())
                break;
            if (current_ >= tokens_.size())
                break;
            if (!match(tokenType))
                break;

            tokens.push_back(std::make_unique<AstNode>(nodeType, current()));

            if (!advance())
                break;

            if (!match(SqlSymbol::COMMA))
            {
                current_--;
                break;
            }
        }

        return tokens;
    }

    CreateDbStatement
    SqlParser::parse_create_db()
    {
        CreateDbStatement stmt;

        match_or_throw(SqlKeyword::DATABASE);
        advance_or_throw("Expected database identifier");

        stmt.name = current();

        return stmt;
    }

    auto
    SqlParser::parse_table_identifier() -> TableIdentifier
    {
        match_or_throw(SqlTokenType::IDENTIFIER);
        SqlToken first_token = current();

        advance();

        if (match(SqlSymbol::PERIOD))
        {
            advance_or_throw("Expected table name after schema");
            match_or_throw(SqlTokenType::IDENTIFIER, "Expected table identifier after schema name");
            SqlToken second_token = current();
            advance();
            return TableIdentifier(second_token, first_token);
        }

        return TableIdentifier(first_token, std::nullopt);
    }

    auto
    SqlParser::parse_create_schema() -> CreateSchemaStatement
    {
        CreateSchemaStatement stmt;

        match_or_throw(SqlKeyword::SCHEMA);
        advance_or_throw("Expected schema identifier");

        stmt.name = current();
        return stmt;
    }

} // namespace sql