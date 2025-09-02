#pragma once

#include "lexer.hpp"
#include <optional>
#include <memory>
#include <utility>

namespace sql {
    enum class AstNodeType {
        IDENTIFIER = 1,
        TABLE_IDENTIFIER,
        COLUMN_IDENTIFIER,
        LITERAL,
        BINARY_EXPR,
        SELECT,
        INSERT,
        UPDATE,
        DELETE,
        CREATE_TABLE,
        CREATE_DATABASE
    };

    enum class AstOperator {
        OR = 1,
        AND,
        NOT,

        EQ,
        NEQ,

        GR,
        GRE,
        LT,
        LTE,

        ASSIGN,
    };

    inline auto
    get_ast_operators_priorities() -> const std::unordered_map<AstOperator, int>& {
        static std::unordered_map<AstOperator, int> map = {
            {AstOperator::OR, 1},
            {AstOperator::AND, 2},
            {AstOperator::NOT, 3},
            {AstOperator::EQ, 4},
            {AstOperator::NEQ, 4},
            {AstOperator::GR, 4},
            {AstOperator::GRE, 4},
            {AstOperator::LT, 4},
            {AstOperator::LTE, 4},
            {AstOperator::ASSIGN, 4},
        };

        return map;
    }

    struct AstNode;

    struct BinaryExpr {
        AstOperator op;
        std::unique_ptr<AstNode> left;
        std::unique_ptr<AstNode> right;
    };

    struct TableIdentifier {
        SqlToken table_name;
        std::optional<SqlToken> schema_name;

        TableIdentifier() = default;

        explicit TableIdentifier(SqlToken table_name,
                        std::optional<SqlToken> schema_name = std::nullopt)
            : table_name(std::move(table_name)), schema_name(std::move(schema_name)) {
        }
    };

    struct SelectStatement {
        TableIdentifier table;
        std::vector<SqlToken> columns;
        std::unique_ptr<AstNode> where;
    };

    struct InsertStatement {
        TableIdentifier table;
        std::vector<SqlToken> columns;
        std::vector<SqlToken> values;
    };

    struct UpdateStatement {
        TableIdentifier table;
        std::vector<AstNode> assignments;
        std::unique_ptr<AstNode> where;
    };

    struct DeleteStatement {
        TableIdentifier table;
        std::unique_ptr<AstNode> where;
    };

    struct ColumnDefinition {
        SqlToken name;
        SqlToken type;
        std::vector<SqlToken> constraints;
    };

    struct CreateTableStatement {
        TableIdentifier table;
        std::vector<ColumnDefinition> columns;
    };

    struct CreateDbStatement {
        SqlToken name;
    };

    using AstNodeValue = std::variant<SqlToken,
                                      BinaryExpr,
                                      SelectStatement,
                                      InsertStatement,
                                      UpdateStatement,
                                      DeleteStatement,
                                      CreateTableStatement,
                                      CreateDbStatement,
                                      ColumnDefinition>;

    struct AstNode {
        AstNodeType type;
        AstNodeValue value;

        AstNode() = default;
        AstNode(AstNodeType type, AstNodeValue&& value);
    };

    class SqlParser {
      public:
        SqlParser(std::vector<SqlToken> tokens);

        auto
        parse() -> std::unique_ptr<AstNode>;

      private:
        std::vector<SqlToken> tokens_;
        size_t current_;

        auto
        parse_select() -> SelectStatement;

        auto
        parse_insert() -> InsertStatement;

        auto
        parse_update() -> UpdateStatement;

        auto
        parse_delete() -> DeleteStatement;

        auto
        parse_create_table() -> CreateTableStatement;

        auto
        parse_create_db() -> CreateDbStatement;

        auto
        parse_binary(int min_priority) -> std::unique_ptr<AstNode>;

        template <typename TEnum>
        auto
        match(const TEnum&) const -> bool;

        template <typename TEnum>
        auto
        match_or_throw(TEnum expected, std::string error_msg = "Invalid statement syntax") const -> bool;

        auto
        advance() noexcept -> bool;

        auto
        advance_or_throw(std::string error_msg = "Invalid statement syntax") -> bool;

        [[nodiscard]] auto
        previous() const noexcept -> const SqlToken&;

        [[nodiscard]] auto
        current() const -> const SqlToken&;

        auto
        parse_primary() -> std::unique_ptr<AstNode>;

        auto
        parse_assignments() -> std::vector<std::unique_ptr<AstNode>>;

        auto
        parse_tokens_list(SqlTokenType tokenType, AstNodeType nodeType) -> std::vector<std::unique_ptr<AstNode>>;

        auto
        parse_column_def() -> ColumnDefinition;

        auto
        parse_table_identifier() -> TableIdentifier;
    };
} // namespace sql
