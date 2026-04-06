//
// Created by poproshaikin on 09.11.25.
//

#include "parser.hpp"

#include "../misc/include/exceptions.hpp"
#include "../misc/include/logger.hpp"

using namespace types;

namespace sql
{

    SqlParser::SqlParser(std::vector<SqlToken> tokens) : tokens_(std::move(tokens)), current_(0)
    {
    }

    void
    SqlParser::set_tokens(const std::vector<SqlToken>& tokens)
    {
        tokens_ = tokens;
    }

    bool
    SqlParser::advance() noexcept
    {
        if (current_ + 1 < tokens_.size())
        {
            current_++;
            return true;
        }

        return false;
    }

    bool
    SqlParser::advance_or_throw(const std::string& error_msg)
    {
        if (!advance())
            throw std::runtime_error(error_msg);

        return true;
    }

    const SqlToken&
    SqlParser::current() const
    {
        if (current_ >= tokens_.size())
            throw std::out_of_range("Parser: current() out of bounds");

        return tokens_[current_];
    }

    const SqlToken&
    SqlParser::previous() const noexcept
    {
        return tokens_[current_ - 1];
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

            if (match(SqlKeyword::INDEX) || match(SqlKeyword::UNIQUE))
                return AstNode(AstNodeType::CREATE_INDEX, parse_create_index());
        }
        if (match(SqlKeyword::DROP))
        {
            if (!advance())
                throw InvalidStatementSyntax();

            if (match(SqlKeyword::INDEX))
                return AstNode(AstNodeType::DROP_INDEX, parse_drop_index());
        }

        throw std::runtime_error("Unsupported statement");
    }

    void
    SqlParser::reset()
    {
        tokens_.clear();
        current_ = 0;
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

        if (match(SqlKeyword::WHERE))
        {
            advance_or_throw();
            stmt.where = parse_binary_expr(0);
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

        ValuesExpr values{};
        while (true)
        {
            advance_or_throw("Invalid statement syntax");
            if (!match(SqlTokenType::LITERAL))
                break;

            values.values.push_back(current());

            if (!advance() || !match(SqlSymbol::COMMA))
            {
                current_--;
                break;
            }
        }
        stmt.values.push_back(values);

        advance_or_throw();
        match_or_throw(SqlSymbol::RPAREN, "Expected right parenthesis");

        return stmt;
    }

    UpdateStatement
    SqlParser::parse_update()
    {
        UpdateStatement stmt;

        advance_or_throw();
        stmt.table = parse_table_identifier();

        match_or_throw(SqlKeyword::SET, "Expected 'SET' keyword");

        while (true)
        {
            advance_or_throw();
            BinaryExpr expr = parse_binary_expr(0);

            if (expr.op != AstOperator::ASSIGN)
            {
                misc::Logger::warn("[parse_update] ERROR: Expected assignment expression (col = value)");
                throw std::runtime_error("Expected assignment expression (col = value)");
            }

            stmt.assignments.emplace_back(std::move(expr));

            if (!match(SqlSymbol::COMMA))
                break;
        }

        if (match(SqlKeyword::WHERE))
        {
            advance();
            stmt.where = parse_binary_expr(0);
        }

        return stmt;
    }

    DeleteStatement
    SqlParser::parse_delete()
    {
        DeleteStatement stmt;

        advance_or_throw();
        match_or_throw(SqlKeyword::FROM, "Expected 'FROM' keyword after DELETE");

        advance_or_throw();
        stmt.table = parse_table_identifier();

        if (match(SqlKeyword::WHERE))
        {
            advance();
            stmt.where = parse_binary_expr(0);
        }

        return stmt;
    }

    CreateTableStatement
    SqlParser::parse_create_table()
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
                    stop = true;

                advance();
            }
        }

        return stmt;
    }

    ColumnDefinition
    SqlParser::parse_column_def()
    {
        ColumnDefinition def;

        match_or_throw(SqlTokenType::IDENTIFIER, "Expected column identifier");
        def.name = current();

        advance_or_throw();

        def.type = current();
        advance_or_throw();

        if (match(SqlSymbol::COMMA) || match(SqlSymbol::RPAREN))
            return def;

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
    }

    std::vector<AstNode>
    SqlParser::parse_tokens_list(SqlTokenType tokenType, AstNodeType nodeType)
    {
        std::vector<AstNode> tokens;

        while (true)
        {
            if (!advance())
                break;
            if (current_ >= tokens_.size())
                break;
            if (!match(tokenType))
                break;

            tokens.emplace_back(nodeType, current());

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

    TableIdentifier
    SqlParser::parse_table_identifier()
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

    CreateSchemaStatement
    SqlParser::parse_create_schema()
    {
        CreateSchemaStatement stmt;

        match_or_throw(SqlKeyword::SCHEMA);
        advance_or_throw("Expected schema identifier");

        stmt.name = current();
        return stmt;
    }

    CreateIndexStatement
    SqlParser::parse_create_index()
    {
        CreateIndexStatement stmt;

        if (match(SqlKeyword::UNIQUE))
        {
            stmt.is_unique = true;
            advance_or_throw("Expected 'INDEX' after 'UNIQUE'");
        }

        match_or_throw(SqlKeyword::INDEX);
        advance_or_throw("Expected index identifier");

        match_or_throw(SqlTokenType::IDENTIFIER, "Expected index identifier");
        stmt.index_name = current();
        advance_or_throw("Incomplete CREATE INDEX syntax");

        match_or_throw(SqlKeyword::ON);
        advance_or_throw("Expected table name after 'ON'");

        stmt.table = parse_table_identifier();

        match_or_throw(SqlSymbol::LPAREN, "Expected '(' after table identifier");
        advance_or_throw("Expected column name after '('");

        match_or_throw(SqlTokenType::IDENTIFIER, "Expected column name after '('");
        stmt.column_name = current();
        advance_or_throw("Expected ')' after column identifier");

        match_or_throw(SqlSymbol::RPAREN, "Expected ')' after column identifier");

        return stmt;
    }

    DropIndexStatement
    SqlParser::parse_drop_index()
    {
        DropIndexStatement stmt;

        match_or_throw(SqlKeyword::INDEX);
        advance_or_throw("Expected index identifier");

        match_or_throw(SqlTokenType::IDENTIFIER, "Expected index identifier");
        stmt.index_name = current();
        advance_or_throw("Expected 'ON' after index identifier");

        match_or_throw(SqlKeyword::ON, "Expected 'ON' after index identifier");
        advance_or_throw("Expected table identifier after 'ON'");
        stmt.table = parse_table_identifier();

        return stmt;
    }

    static std::optional<AstOperator>
    to_ast_operator(SqlOperator type)
    {
        switch (type)
        {
        case SqlOperator::OR:
            return AstOperator::OR;
        case SqlOperator::AND:
            return AstOperator::AND;
        case SqlOperator::EQ:
            return AstOperator::EQ;
        case SqlOperator::NEQ:
            return AstOperator::NEQ;
        case SqlOperator::NOT:
            return AstOperator::NOT;
        case SqlOperator::GR:
            return AstOperator::GR;
        case SqlOperator::GRE:
            return AstOperator::GRE;
        case SqlOperator::LT:
            return AstOperator::LT;
        case SqlOperator::LTE:
            return AstOperator::LTE;
        case SqlOperator::IS:
            return AstOperator::IS;
        case SqlOperator::ASSIGN:
            return AstOperator::ASSIGN;
        default:
            return std::nullopt;
        }
    }

    std::unique_ptr<AstNode>
    SqlParser::parse_binary_tree(int min_priority)
    {
        auto left = parse_primary();

        while (true)
        {
            const SqlToken& token = current();
            std::optional<AstOperator> op_opt;
            if (token.type == SqlTokenType::OPERATOR)
            {
                op_opt = to_ast_operator(std::get<SqlOperator>(token.detail));
            }
            else if (token.type == SqlTokenType::KEYWORD &&
                     std::get<SqlKeyword>(token.detail) == SqlKeyword::IS)
            {
                op_opt = AstOperator::IS;
            }
            else
            {
                break;
            }

            if (!op_opt)
                break;

            int priority = ast_operators_priorities().at(*op_opt);

            if (priority < min_priority)
                break;

            AstOperator op = *op_opt;
            advance();

            if (op == AstOperator::IS && match(SqlKeyword::NOT))
            {
                op = AstOperator::NEQ;
                advance();
            }

            auto right = parse_binary_tree(priority + 1);

            BinaryExpr expr;
            expr.op = op;
            expr.left = std::move(left);
            expr.right = std::move(right);

            left =
                std::make_unique<AstNode>(AstNodeType::BINARY_EXPR, AstNodeValue(std::move(expr)));

        }

        return left;
    }

    BinaryExpr
    SqlParser::parse_binary_expr(int min_priority)
    {
        auto node = parse_binary_tree(min_priority);

        if (!node)
            throw std::runtime_error("Expected binary expression, got null node");

        if (node->type != AstNodeType::BINARY_EXPR)
            throw std::runtime_error("Expected binary expression");

        return std::get<BinaryExpr>(std::move(node->value));
    }

    std::unique_ptr<AstNode>
    SqlParser::parse_primary()
    {
        const SqlToken& token = current();

        if (match(SqlSymbol::LPAREN))
        {
            advance();
            auto node = parse_binary_expr(0);
            if (!match(SqlSymbol::RPAREN))
                throw std::runtime_error("Expected right parenthesis");

            return std::make_unique<AstNode>(AstNodeType::BINARY_EXPR, std::move(node));
        }

        if (match(SqlOperator::NOT))
        {
            advance();
            auto right = parse_primary();

            BinaryExpr expr;
            expr.op = AstOperator::NOT;
            expr.left = nullptr;
            expr.right = std::move(right);

            return std::make_unique<AstNode>(AstNodeType::BINARY_EXPR, std::move(expr));
        }

        if (match(SqlKeyword::_NULL))
        {
            advance();

            SqlToken copy = token;
            copy.type = SqlTokenType::LITERAL;
            copy.detail = SqlLiteral::NULL_;

            return std::make_unique<AstNode>(AstNodeType::LITERAL, AstNodeValue(std::move(copy)));
        }

        if (match(SqlTokenType::LITERAL))
        {
            advance();
            SqlToken copy = token;
            return std::make_unique<AstNode>(AstNodeType::LITERAL, AstNodeValue(std::move(copy)));
        }

        if (match(SqlTokenType::IDENTIFIER))
        {
            advance();
            SqlToken copy = token;
            return std::make_unique<AstNode>(
                AstNodeType::IDENTIFIER, AstNodeValue(std::move(copy))
            );
        }

        char buffer[256];
        snprintf(buffer, 256, "Unexpected token in expression: %s", token.to_string().data());
        throw std::runtime_error(buffer);
    }
} // namespace sql