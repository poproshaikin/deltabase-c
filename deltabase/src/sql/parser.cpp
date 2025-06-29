#include "include/parser.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>

namespace sql {
    AstNode::AstNode(AstNodeType type, AstNodeValue&& value) :
        type(type), value(std::move(value)) { }

    SqlParser::SqlParser(std::vector<SqlToken> tokens) : _tokens(std::move(tokens)), _current(tokens.size()) { }

    bool SqlParser::advance() noexcept {
        if (_current + 1 < _tokens.size()) {
            _current++;
            return true;
        }

        return false;
    }

    bool SqlParser::advance_or_throw(std::string error_msg) {
        if (!advance()) {
            throw std::runtime_error(error_msg);
        }
        return true;
    }

    template<typename T, typename Variant>
    struct is_in_variant;

    template<typename T, typename... Ts>
    struct is_in_variant<T, std::variant<Ts...>> {
        static constexpr bool value = (std::is_same_v<T, Ts> || ...);
    };

    template<typename T, typename Variant>
    constexpr bool is_in_variant_v = is_in_variant<T, Variant>::value;

    template<typename TEnum>
    bool SqlParser::match(const TEnum& expected) const {
        if constexpr (
            std::is_same_v<TEnum, SqlTokenType>
        ) {
            return _tokens[_current].type == expected; 
        }
        else if constexpr (
            std::is_same_v<TEnum, std::monostate> || 
            std::is_same_v<TEnum, SqlOperator> || 
            std::is_same_v<TEnum, SqlSymbol> ||
            std::is_same_v<TEnum, SqlKeyword> ||
            std::is_same_v<TEnum, SqlLiteral>
        ) {
            const auto* value = std::get_if<TEnum>(&_tokens[_current].detail);
            return value && *value == expected;
        } 
        else {
            throw std::runtime_error("Unsupported type passed for match()");
        }
    }

    template<typename TEnum>
    bool SqlParser::match_or_throw(TEnum expected, std::string error_msg) const {
        if (!match<TEnum>(expected)) {
            throw std::runtime_error(error_msg);
        }
        return true;
    }

    const SqlToken& SqlParser::current() const {
        if (_current >= _tokens.size()) {
            throw std::out_of_range("Parser: current() out of bounds");
        }
        return _tokens[_current];
    }

    std::unique_ptr<AstNode> SqlParser::parse() {
        ;

        if (match(SqlKeyword::SELECT)) {
            AstNodeValue value = std::move(parse_select());
            return std::make_unique<AstNode>(AstNodeType::SELECT, AstNodeValue(std::move(value)));
        }
        else if (match(SqlKeyword::INSERT)) {
            AstNodeValue value = std::move(parse_insert());
            return std::make_unique<AstNode>(AstNodeType::INSERT, AstNodeValue(std::move(value)));
        }
        else if (match(SqlKeyword::UPDATE)) {
            AstNodeValue value = std::move(parse_update());
            return std::make_unique<AstNode>(AstNodeType::UPDATE, AstNodeValue(std::move(value)));
        }
        else if (match(SqlKeyword::DELETE)) {
            AstNodeValue value = std::move(parse_delete());
            return std::make_unique<AstNode>(AstNodeType::DELETE, AstNodeValue(std::move(value)));
        }
        else {
            throw std::runtime_error("Unsupported statement");
        }

    }

    SelectStatement SqlParser::parse_select() {
        SelectStatement stmt;

        if (!match(SqlOperator::MUL)) {
            stmt.columns = parse_tokens_list(SqlTokenType::IDENTIFIER, AstNodeType::COLUMN_IDENTIFIER);
        }
        else {
            advance();
        }

        if (!advance()) {
            throw std::runtime_error("Invalid statement syntax");
        }
        if (!match(SqlKeyword::FROM)) {
            throw std::runtime_error("Expected 'FROM'");  
        }

        if (!advance()) {
            throw std::runtime_error("Invalid statement syntax");
        }
        if (!match(SqlTokenType::IDENTIFIER)) {
            throw std::runtime_error("Expected name of the table");
        }
        stmt.table = std::make_unique<AstNode>(AstNodeType::TABLE_IDENTIFIER, current());

        if (advance() && match(SqlKeyword::WHERE)) {
            if (!advance()) {
                throw std::runtime_error("Invalid statement syntax");
            }
            stmt.where = parse_binary(0);
        }

        return stmt;
    }

    InsertStatement SqlParser::parse_insert() { 
        InsertStatement stmt;

        advance_or_throw();
        match_or_throw(SqlKeyword::INTO, "Expected 'INTO' keyword");

        advance_or_throw();
        match_or_throw(SqlTokenType::IDENTIFIER, "Expected table identifier");

        stmt.table = std::make_unique<AstNode>(AstNodeType::TABLE_IDENTIFIER, current());  

        advance_or_throw();
        match_or_throw(SqlSymbol::LPAREN, "Expected left parenthesis");

        if (!match(SqlOperator::MUL)) {
            stmt.columns = parse_tokens_list(SqlTokenType::IDENTIFIER, AstNodeType::COLUMN_IDENTIFIER);
        }
        else {
            advance_or_throw("Invalid statement syntax");
        }

        advance_or_throw();
        match_or_throw(SqlSymbol::RPAREN, "Expected right parenthesis");

        advance_or_throw();
        match_or_throw(SqlKeyword::VALUES, "Expected 'VALUES' keyword");

        advance_or_throw();
        match_or_throw(SqlSymbol::LPAREN, "Expected left parenthesis");

        stmt.values = parse_tokens_list(SqlTokenType::LITERAL, AstNodeType::LITERAL);

        advance_or_throw();
        match_or_throw(SqlSymbol::RPAREN, "Expected right parenthesis");

        return stmt;
    }

    UpdateStatement SqlParser::parse_update() {
        UpdateStatement stmt;

        advance_or_throw();
        match_or_throw(SqlTokenType::IDENTIFIER, "Expected table identifier after UPDATE");

        stmt.table = std::make_unique<AstNode>(AstNodeType::TABLE_IDENTIFIER, current());

        advance_or_throw();
        match_or_throw(SqlKeyword::SET, "Expected 'SET' keyword");

        while (true) {
            advance_or_throw(); 
            std::unique_ptr<AstNode> expr = parse_binary(0);

            if (!expr || expr->type != AstNodeType::BINARY_EXPR) {
                throw std::runtime_error("Expected assignment expression (col = value)");
            }

            BinaryExpr* assignment = std::get_if<BinaryExpr>(&expr->value);
            if (!assignment || assignment->op != AstOperator::ASSIGN) {
                throw std::runtime_error("Expected '=' in assignment");
            }

            stmt.assignments.push_back(std::make_unique<AstNode>(AstNodeType::BINARY_EXPR, std::move(*assignment)));

            if (!match(SqlSymbol::COMMA)) {
                break;
            }
        }

        if (match(SqlKeyword::WHERE)) {
            advance();
            stmt.where = parse_binary(0); 
        }

        return stmt;
    }

    DeleteStatement SqlParser::parse_delete() {
        DeleteStatement stmt;

        advance_or_throw();
        match_or_throw(SqlKeyword::FROM, "Expected 'FROM' keyword after DELETE");

        advance_or_throw();
        match_or_throw(SqlTokenType::IDENTIFIER, "Expected table identifier after FROM");

        stmt.table = std::make_unique<AstNode>(AstNodeType::TABLE_IDENTIFIER, current());

        advance();
        if (match(SqlKeyword::WHERE)) {
            advance();
            stmt.where = parse_binary(0);
        }

        return stmt;
    }

    std::vector<std::unique_ptr<AstNode>> SqlParser::parse_tokens_list(SqlTokenType tokenType, AstNodeType nodeType) {
        std::vector<std::unique_ptr<AstNode>> tokens;

        while (true) {
            if (!advance()) break;
            if (_current >= _tokens.size()) break;
            const SqlToken& token = current();

            if (!match(tokenType)) {
                break;
            }

            tokens.push_back(std::make_unique<AstNode>(nodeType, current()));

            if (!advance()) break;

            if (!match(SqlSymbol::COMMA)) {
                _current--;
                break;
            }
        }

        return tokens;
    }
}
