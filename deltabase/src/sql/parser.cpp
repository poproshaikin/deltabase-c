#include "include/parser.hpp"
#include <memory>
#include <stdexcept>
#include <vector>

using namespace sql;
        
AstNode::AstNode(AstNodeType type, AstNodeValue&& value) :
    type(type), value(std::move(value)) { }

SqlParser::SqlParser(std::vector<SqlToken> tokens) {
    tokens = tokens;
}

bool SqlParser::advance() noexcept {
    if (_current < _tokens.size() - 1) {
        _current++;
        return true;
    }
    
    return false;
}

bool SqlParser::match(SqlTokenType type) const noexcept {
    bool matches = _tokens[_current].type == type;
    return matches;
}

bool SqlParser::match(SqlKeyword keyword) const noexcept {
    bool matches = std::get<SqlKeyword>(_tokens[_current].detail) == keyword;
    return matches;
}

bool SqlParser::match(SqlOperator op) const noexcept {
    bool matches = std::get<SqlOperator>(_tokens[_current].detail) == op;
    return matches;
}

bool SqlParser::match(SqlSymbol sym) const noexcept {
    bool matches = std::get<SqlSymbol>(_tokens[_current].detail) == sym;
    return matches;
}

const SqlToken& SqlParser::current() const noexcept {
    const auto& token = _tokens[_current];  
    return token;
}

std::unique_ptr<AstNode> SqlParser::parse() {
    AstNodeValue value;
    
    if (match(SqlKeyword::SELECT)) {
        value = std::move(parse_select());
    } 
    else if (match(SqlKeyword::INSERT)) {
        value = parse_insert();
    }

    throw std::runtime_error("Unsupported statement");
}

std::vector<std::unique_ptr<AstNode>> SqlParser::parse_columns() {
    std::vector<std::unique_ptr<AstNode>> columns;

    while (true) {
        advance();
        const SqlToken& token = current();

        if (token.type != SqlTokenType::IDENTIFIER) {
            break;
        }

        auto ast_node_ptr = std::make_unique<AstNode>(AstNodeType::COLUMN_IDENTIFIER, AstNodeValue{token});
        columns.push_back(std::move(ast_node_ptr));
    }

    return columns;
}

SelectStatement SqlParser::parse_select() {
    SelectStatement stmt;

    advance();
    if (!match(SqlOperator::MUL)) {
        stmt.columns = parse_columns();
    }

    advance(); 
    if (!match(SqlKeyword::FROM)) {
        throw std::runtime_error("Expected 'FROM'");  
    }

    advance();
    if (!match(SqlTokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected name of the table");
    }
    stmt.table = std::make_unique<AstNode>(AstNodeType::TABLE_IDENTIFIER, current());

    advance();
    if (match(SqlKeyword::WHERE)) {
        advance();
        stmt.where = parse_binary(0);
    }

    return stmt;
}

InsertStatement SqlParser::parse_insert() {
    
}
