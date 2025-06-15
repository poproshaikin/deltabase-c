#include "include/parser.hpp"
#include <optional>
#include <stdexcept>

using namespace sql;

std::optional<AstOperator> to_ast_operator(SqlOperator type) {

    switch (type) {
        case SqlOperator::OR: return AstOperator::OR;
        case SqlOperator::AND: return AstOperator::AND;
        case SqlOperator::NOT: return AstOperator::NOT;
        case SqlOperator::GR: return AstOperator::GR;
        case SqlOperator::GRE: return AstOperator::GRE;
        case SqlOperator::LT: return AstOperator::LT;
        case SqlOperator::LTE: return AstOperator::LTE;
        default: return std::nullopt;
    }
}

std::unique_ptr<AstNode> SqlParser::parse_primary() {
    const SqlToken& token = current();

    if (token.type == SqlTokenType::SYMBOL && std::get<SqlSymbol>(token.detail) == SqlSymbol::LPAREN) {
        advance();
        auto node = parse_binary(0);
        if (!match(SqlSymbol::RPAREN)) {
            throw std::runtime_error("Expected right parenthesis");
        }
        return node;
    }

    // NOT expr
    if (token.type == SqlTokenType::OPERATOR && std::get<SqlOperator>(token.detail) == SqlOperator::NOT) {
        advance();
        auto right = parse_primary();

        BinaryExpr expr {
            .op = AstOperator::NOT,
            .left = nullptr,
            .right = std::move(right)
        };

        return std::make_unique<AstNode>(
            AstNodeType::BINARY_EXPR,
            std::move(expr)
        );
    }

    // identifiers and literals
    if (token.type == SqlTokenType::IDENTIFIER || token.type == SqlTokenType::LITERAL) {
        SqlToken copy = token;
        advance();

        return std::make_unique<AstNode>(
            AstNodeType::LITERAL,
            AstNodeValue(std::move(copy))
        );
    }

    throw std::runtime_error("Unexpected token in expression");
}


std::unique_ptr<AstNode> SqlParser::parse_binary(int min_priority) {
    auto left = parse_primary();

    while (true) {
        const SqlToken& token = current();
        if (token.type != SqlTokenType::OPERATOR) {
            break;
        }
        auto op_opt = to_ast_operator(std::get<SqlOperator>(token.detail));
        if (!op_opt) break;

        int priority = getAstOperatorsPriorities().at(*op_opt);
        if (priority < min_priority) break;

        AstOperator op = *op_opt;
        advance();

        auto right = parse_binary(priority + 1);

        BinaryExpr expr {
            .op = op,
            .left = std::move(left),
            .right = std::move(right)
        };

        left = std::make_unique<AstNode>(
            AstNodeType::BINARY_EXPR,
            AstNodeValue(std::move(expr))
        );
    }

    return left;
} 


