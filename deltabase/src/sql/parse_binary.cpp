#include "include/parser.hpp"
#include <optional>
#include <stdexcept>

using namespace sql;

auto
to_ast_operator(SqlOperator type) -> std::optional<AstOperator> {
    switch (type) {
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
    case SqlOperator::ASSIGN:
        return AstOperator::ASSIGN;
    default:
        return std::nullopt;
    }
}

BinaryExpr
SqlParser::parse_binary(int min_priority)
{
    std::unique_ptr<AstNode> left = parse_primary();

    while (true)
    {
        const SqlToken& token = current();
        if (token.type != SqlTokenType::OPERATOR)
            break;

        auto op_opt = to_ast_operator(std::get<SqlOperator>(token.detail));
        if (!op_opt)
            break;

        int priority = ast_operators_priorities().at(*op_opt);
        if (priority < min_priority)
            break;

        AstOperator op = *op_opt;
        advance();

        BinaryExpr right = parse_binary(priority + 1);

        BinaryExpr expr{
            .op = op,
            .left = std::move(left),
            .right =
                std::make_unique<AstNode>(AstNodeType::BINARY_EXPR, AstNodeValue(std::move(right)))
        };

        left = std::make_unique<AstNode>(AstNodeType::BINARY_EXPR, AstNodeValue(std::move(expr)));
    }

    if (left->type == AstNodeType::BINARY_EXPR)
        return std::get<BinaryExpr>(std::move(left->value));

    BinaryExpr result{
        .op = AstOperator::NONE,
        .left = std::move(left),
        .right = nullptr
    };
    return result;
}

std::unique_ptr<AstNode>
SqlParser::parse_primary()
{
    const SqlToken& token = current();

    if (match(SqlSymbol::LPAREN))
    {
        advance();
        auto node = parse_binary(0);
        if (!match(SqlSymbol::RPAREN))
            throw std::runtime_error("Expected right parenthesis");

        return std::make_unique<AstNode>(AstNodeType::BINARY_EXPR, std::move(node));
    }

    if (match(SqlOperator::NOT))
    {
        advance();
        auto right = parse_primary();

        BinaryExpr expr{.op = AstOperator::NOT, .left = nullptr, .right = std::move(right)};

        return std::make_unique<AstNode>(AstNodeType::BINARY_EXPR, std::move(expr));
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
        return std::make_unique<AstNode>(AstNodeType::IDENTIFIER, AstNodeValue(std::move(copy)));
    }

    char buffer[256];
    snprintf(buffer, 256, "Unexpected token in expression: %s", token.to_string().data());
    throw std::runtime_error(buffer);
}
