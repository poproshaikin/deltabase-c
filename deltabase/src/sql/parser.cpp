#include "include/parser.hpp"
#include "../misc/include/exceptions.hpp"
#include "include/lexer.hpp"
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>

namespace sql {
    AstNode::AstNode(AstNodeType type, AstNodeValue&& value) : type(type), value(std::move(value)) {
    }

    SqlParser::SqlParser(std::vector<SqlToken> tokens) : tokens_(std::move(tokens)), current_(0) {
    }

    auto
    SqlParser::advance() noexcept -> bool {
        if (current_ + 1 < tokens_.size()) {
            current_++;
            return true;
        }

        return false;
    }

    auto
    SqlParser::advance_or_throw(std::string error_msg) -> bool {
        if (!advance()) {
            throw std::runtime_error(error_msg);
        }
        return true;
    }

    template <typename T, typename Variant> struct is_in_variant;

    template <typename T, typename... Ts> struct is_in_variant<T, std::variant<Ts...>> {
        static constexpr bool value = (std::is_same_v<T, Ts> || ...);
    };

    template <typename T, typename Variant>
    constexpr bool is_in_variant_v = is_in_variant<T, Variant>::value;

    template <typename TEnum>
    auto
    SqlParser::match(const TEnum& expected) const -> bool {
        if (current_ >= tokens_.size()) {
            return false;
        }

        if constexpr (std::is_same_v<TEnum, SqlTokenType>) {
            return tokens_[current_].type == expected;
        } else if constexpr (std::is_same_v<TEnum, std::monostate> ||
                             std::is_same_v<TEnum, SqlOperator> ||
                             std::is_same_v<TEnum, SqlSymbol> ||
                             std::is_same_v<TEnum, SqlKeyword> ||
                             std::is_same_v<TEnum, SqlLiteral>) {
            const auto* value = std::get_if<TEnum>(&tokens_[current_].detail);
            return value && *value == expected;
        } else {
            throw std::runtime_error("Unsupported type passed for match()");
        }
    }

    template <typename TEnum>
    auto
    SqlParser::match_or_throw(TEnum expected, std::string error_msg) const -> bool {
        if (!match<TEnum>(expected)) {
            throw std::runtime_error(error_msg);
        }
        return true;
    }

    auto
    SqlParser::current() const -> const SqlToken& {
        if (current_ >= tokens_.size()) {
            throw std::out_of_range("Parser: current() out of bounds");
        }
        return tokens_[current_];
    }

    auto
    SqlParser::parse() -> std::unique_ptr<AstNode> {
        if (match(SqlKeyword::SELECT)) {
            return std::make_unique<AstNode>(AstNodeType::SELECT, parse_select());
        } else if (match(SqlKeyword::INSERT)) {
            return std::make_unique<AstNode>(AstNodeType::INSERT, parse_insert());
        } else if (match(SqlKeyword::UPDATE)) {
            return std::make_unique<AstNode>(AstNodeType::UPDATE, parse_update());
        } else if (match(SqlKeyword::DELETE)) {
            return std::make_unique<AstNode>(AstNodeType::DELETE, parse_delete());
        } else if (match(SqlKeyword::CREATE)) {
            if (!advance()) {
                throw InvalidStatementSyntax();
            }
            if (match(SqlKeyword::TABLE)) {
                return std::make_unique<AstNode>(AstNodeType::CREATE_TABLE, parse_create_table());
            }
            if (match(SqlKeyword::DATABASE)) {
                return std::make_unique<AstNode>(AstNodeType::CREATE_DATABASE, parse_create_db());
            }
            if (match(SqlKeyword::SCHEMA)) {
                return std::make_unique<AstNode>(AstNodeType::CREATE_SCHEMA, parse_create_schema());
            }
        }

        throw std::runtime_error("Unsupported statement");
    }

    auto
    SqlParser::parse_select() -> SelectStatement {
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
        stmt.table = parse_table_identifier();

        if (advance() && match(SqlKeyword::WHERE)) {
            advance_or_throw();
            stmt.where = parse_binary(0);
        }

        return stmt;
    }

    auto
    SqlParser::parse_insert() -> InsertStatement {
        InsertStatement stmt;

        advance_or_throw();
        match_or_throw(SqlKeyword::INTO, "Expected 'INTO' keyword");

        advance_or_throw();
        stmt.table = parse_table_identifier();

        match_or_throw(SqlSymbol::LPAREN, "Expected left parenthesis");

        if (!match(SqlOperator::MUL)) {
            while (true) {
                advance_or_throw("Invalid statement syntax");
                if (!match(SqlTokenType::IDENTIFIER)) {
                    break;
                }
                stmt.columns.push_back(current());

                if (!advance() || !match(SqlSymbol::COMMA)) {
                    current_--;
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
                current_--;
                break;
            }
        }

        advance_or_throw();
        match_or_throw(SqlSymbol::RPAREN, "Expected right parenthesis");

        return stmt;
    }

    auto
    SqlParser::parse_update() -> UpdateStatement {
        UpdateStatement stmt;

        advance_or_throw();
        stmt.table = parse_table_identifier();

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

    auto
    SqlParser::parse_delete() -> DeleteStatement {
        DeleteStatement stmt;

        advance_or_throw();
        match_or_throw(SqlKeyword::FROM, "Expected 'FROM' keyword after DELETE");

        advance_or_throw();
        stmt.table = parse_table_identifier();

        if (match(SqlKeyword::WHERE)) {
            advance();
            stmt.where = parse_binary(0);
        }

        return stmt;
    }

    auto
    SqlParser::parse_create_table() -> CreateTableStatement {
        CreateTableStatement stmt;

        match_or_throw(SqlKeyword::TABLE, "Expected 'TABLE' after 'CREATE'");

        advance_or_throw();
        stmt.table = parse_table_identifier();

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

    auto
    SqlParser::parse_column_def() -> ColumnDefinition {
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

    auto
    SqlParser::parse_tokens_list(SqlTokenType tokenType, AstNodeType nodeType) -> std::vector<std::unique_ptr<AstNode>> {
        std::vector<std::unique_ptr<AstNode>> tokens;

        while (true) {
            if (!advance())
                break;
            if (current_ >= tokens_.size())
                break;
            const SqlToken& token = current();

            if (!match(tokenType)) {
                break;
            }

            tokens.push_back(std::make_unique<AstNode>(nodeType, current()));

            if (!advance())
                break;

            if (!match(SqlSymbol::COMMA)) {
                current_--;
                break;
            }
        }

        return tokens;
    }

    auto
    SqlParser::parse_create_db() -> CreateDbStatement {
        CreateDbStatement stmt;

        match_or_throw(SqlKeyword::DATABASE);
        advance_or_throw("Expected database identifier");

        stmt.name = current();

        return stmt;
    }

    auto
    SqlParser::parse_table_identifier() -> TableIdentifier {
        match_or_throw(SqlTokenType::IDENTIFIER);
        SqlToken first_token = current();

        advance();
        
        if (match(SqlSymbol::PERIOD)) {
            advance_or_throw("Expected table name after schema");
            match_or_throw(SqlTokenType::IDENTIFIER, "Expected table identifier after schema name");
            SqlToken second_token = current();
            advance();
            return TableIdentifier(second_token, first_token);   
        }

        return TableIdentifier(first_token, std::nullopt);
    }

    auto
    SqlParser::parse_create_schema() -> CreateSchemaStatement {
        CreateSchemaStatement stmt;

        match_or_throw(SqlKeyword::SCHEMA);
        advance_or_throw("Expected schema identifier");

        stmt.name = current();
        return stmt;
    }

} // namespace sql
