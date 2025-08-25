#include "include/parser.hpp"
#include "include/lexer.hpp"
#include "../misc/include/exceptions.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>

namespace sql {
    AstNode::AstNode(AstNodeType type, AstNodeValue&& value) :
        type(type), value(std::move(value)) { }

    SqlParser::SqlParser(std::vector<SqlToken> tokens) : _tokens(std::move(tokens)), _current(0) { }

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
        if (_current >= _tokens.size()) {
            return false;
        }
        
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
        if (match(SqlKeyword::SELECT)) {
            return std::make_unique<AstNode>(AstNodeType::SELECT, parse_select());
        }
        else if (match(SqlKeyword::INSERT)) {
            return std::make_unique<AstNode>(AstNodeType::INSERT, parse_insert());
        }
        else if (match(SqlKeyword::UPDATE)) {
            return std::make_unique<AstNode>(AstNodeType::UPDATE, parse_update());
        }
        else if (match(SqlKeyword::DELETE)) {
            return std::make_unique<AstNode>(AstNodeType::DELETE, parse_delete());
        }
        else if (match(SqlKeyword::CREATE)) {
            if (!advance()) {
                throw InvalidStatementSyntax();
            }
            if (match(SqlKeyword::TABLE)) {
                return std::make_unique<AstNode>(AstNodeType::CREATE_TABLE, parse_create_table());
            }
            if (match(SqlKeyword::DATABASE)) {
                return std::make_unique<AstNode>(AstNodeType::CREATE_DATABASE, parse_create_db());
            }
        }
        
        throw std::runtime_error("Unsupported statement");
    }

    SelectStatement SqlParser::parse_select() {
        SelectStatement stmt;

        if (!match(SqlOperator::MUL)) {
            while (true) {
                advance_or_throw("Invalid statement syntax");
                if (!match(SqlTokenType::IDENTIFIER)) {
                    break;
                }
                stmt.columns.push_back(current());
                
                if (!advance() || !match(SqlSymbol::COMMA)) {
                    break;
                }
            }
        } else {
            advance();
        }

        advance_or_throw();
        match_or_throw(SqlKeyword::FROM, "Expected 'FROM'");

        advance_or_throw();
        if (!match(SqlTokenType::IDENTIFIER)) {
            throw std::runtime_error("Expected name of the table");
        }
        stmt.table = current();

        if (advance() && match(SqlKeyword::WHERE)) {
            advance_or_throw();
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

        stmt.table = current();

        advance_or_throw();
        match_or_throw(SqlSymbol::LPAREN, "Expected left parenthesis");

        if (!match(SqlOperator::MUL)) {
            while (true) {
                advance_or_throw("Invalid statement syntax");
                if (!match(SqlTokenType::IDENTIFIER)) {
                    break;
                }
                stmt.columns.push_back(current());
                
                if (!advance() || !match(SqlSymbol::COMMA)) {
                    _current--;
                    break;
                }
            }
        } else {
            advance_or_throw("Invalid statement syntax");
        }

        advance_or_throw();
        match_or_throw(SqlSymbol::RPAREN, "Expected right parenthesis");

        advance_or_throw();
        match_or_throw(SqlKeyword::VALUES, "Expected 'VALUES' keyword");

        advance_or_throw();
        match_or_throw(SqlSymbol::LPAREN, "Expected left parenthesis");

        while (true) {
            advance_or_throw("Invalid statement syntax");
            if (!match(SqlTokenType::LITERAL)) {
                break;
            }
            stmt.values.push_back(current());
            
            if (!advance() || !match(SqlSymbol::COMMA)) {
                _current--;
                break;
            }
        }

        advance_or_throw();
        match_or_throw(SqlSymbol::RPAREN, "Expected right parenthesis");

        return stmt;
    }

    UpdateStatement SqlParser::parse_update() {
        UpdateStatement stmt;

        advance_or_throw();
        match_or_throw(SqlTokenType::IDENTIFIER, "Expected table identifier after UPDATE");

        stmt.table = current();

        advance_or_throw();
        match_or_throw(SqlKeyword::SET, "Expected 'SET' keyword");

        while (true) {
            advance_or_throw(); 
            std::unique_ptr<AstNode> expr = parse_binary(0);

            if (!expr || expr->type != AstNodeType::BINARY_EXPR) {
                throw std::runtime_error("Expected assignment expression (col = value)");
            }

            BinaryExpr assignment = std::get<BinaryExpr>(std::move(expr->value));
            if (assignment.op != AstOperator::ASSIGN) {
                throw std::runtime_error("Expected '=' in assignment");
            }

            stmt.assignments.emplace_back(AstNodeType::BINARY_EXPR, std::move(assignment));

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

        stmt.table = current();

        advance();
        if (match(SqlKeyword::WHERE)) {
            advance();
            stmt.where = parse_binary(0);
        }

        return stmt;
    }

    CreateTableStatement SqlParser::parse_create_table() {
        CreateTableStatement stmt;

        match_or_throw(SqlKeyword::TABLE, "Expected 'TABLE' after 'CREATE'");

        advance_or_throw();
        match_or_throw(SqlTokenType::IDENTIFIER, "Expected name of the table");

        stmt.name = current();

        advance_or_throw();
        if (match(SqlSymbol::LPAREN)) {
            advance_or_throw();
            bool stop = false;
            while (!stop) {
                stmt.columns.push_back(parse_column_def());

                if (match(SqlSymbol::RPAREN)) {
                    stop = true;
                }
                advance();
            }
        }
        
        return stmt;
    }

    ColumnDefinition SqlParser::parse_column_def() {
        ColumnDefinition def;

        match_or_throw(SqlTokenType::IDENTIFIER, "Expected column identifier");
        def.name = current();

        advance_or_throw();
        
        def.type = current();
        advance_or_throw();

        if (match(SqlSymbol::COMMA) || match(SqlSymbol::RPAREN)) {
            return def;
        }

        advance_or_throw();
        
        bool stop = false;
        while (!stop) {
            const SqlToken& cur = current();

            if (match(SqlSymbol::COMMA) || match(SqlSymbol::RPAREN)) {
                return def;
            }

            if (!std::holds_alternative<SqlKeyword>(cur.detail)) {
                throw InvalidStatementSyntax();
            }

            if (!cur.is_constraint() || !cur.is_data_type()) {
                throw InvalidStatementSyntax();
            }

            SqlToken copy = cur;
            def.constraints.push_back(copy);
            advance_or_throw();
        }

        return def;
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

    CreateDbStatement SqlParser::parse_create_db() {
        CreateDbStatement stmt;

        match_or_throw(SqlKeyword::DATABASE);
        advance_or_throw("Expected database identifier");

        stmt.name = current();

        return stmt;
    }
}

