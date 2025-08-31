#pragma once

#include "lexer.hpp"
#include <optional>
#include <memory>

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

    inline const std::unordered_map<AstOperator, int>&
    get_ast_operators_priorities() {
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

        TableIdentifier(SqlToken table_name,
                        std::optional<SqlToken> schema_name = std::nullopt)
            : table_name(table_name), schema_name(schema_name) {
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

        std::unique_ptr<AstNode>
        parse();

      private:
        std::vector<SqlToken> _tokens;
        size_t _current;

        SelectStatement
        parse_select();

        InsertStatement
        parse_insert();

        UpdateStatement
        parse_update();

        DeleteStatement
        parse_delete();

        CreateTableStatement
        parse_create_table();

        CreateDbStatement
        parse_create_db();

        std::unique_ptr<AstNode>
        parse_binary(int min_priority);

        template <typename TEnum>
        bool
        match(const TEnum&) const;

        template <typename TEnum>
        bool
        match_or_throw(TEnum expected, std::string error_msg = "Invalid statement syntax") const;

        bool
        advance() noexcept;

        bool
        advance_or_throw(std::string error_msg = "Invalid statement syntax");

        const SqlToken&
        previous() const noexcept;

        const SqlToken&
        current() const;

        std::unique_ptr<AstNode>
        parse_primary();

        std::vector<std::unique_ptr<AstNode>>
        parse_assignments();

        std::vector<std::unique_ptr<AstNode>>
        parse_tokens_list(SqlTokenType tokenType, AstNodeType nodeType);

        ColumnDefinition
        parse_column_def();

        TableIdentifier
        parse_table_identifier();
    };
} // namespace sql
