#pragma once

#include "lexer.hpp"
#include <optional>
#include <memory>
#include <utility>

namespace sql
{
    enum class AstNodeType
    {
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
        CREATE_DATABASE,
        CREATE_SCHEMA,
    };

    enum class AstOperator
    {
        NONE = 0,
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
    ast_operators_priorities()
    {
        static std::unordered_map<AstOperator, int> map =
        {
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

    struct BinaryExpr
    {
        AstOperator op;
        std::unique_ptr<AstNode> left;
        std::unique_ptr<AstNode> right;
    };

    struct TableIdentifier
    {
        SqlToken table_name;
        std::optional<SqlToken> schema_name;

        TableIdentifier() = default;

        explicit
        TableIdentifier(
            SqlToken table_name,
            std::optional<SqlToken> schema_name = std::nullopt
        )
            : table_name(std::move(table_name)), schema_name(std::move(schema_name))
        {
        }
    };

    struct SelectStatement
    {
        TableIdentifier table;
        std::vector<SqlToken> columns;
        std::optional<BinaryExpr> where;
    };

    struct InsertStatement
    {
        TableIdentifier table;
        std::vector<SqlToken> columns;
        std::vector<SqlToken> values;
    };

    struct UpdateStatement
    {
        TableIdentifier table;
        std::vector<BinaryExpr> assignments;
        std::optional<BinaryExpr> where;
    };

    struct DeleteStatement
    {
        TableIdentifier table;
        std::optional<BinaryExpr> where;
    };

    struct ColumnDefinition
    {
        SqlToken name;
        SqlToken type;
        std::vector<SqlToken> constraints;
    };

    struct CreateTableStatement
    {
        TableIdentifier table;
        std::vector<ColumnDefinition> columns;
    };

    struct CreateSchemaStatement
    {
        SqlToken name;
    };

    struct CreateDbStatement
    {
        SqlToken name;
    };

    using AstNodeValue = std::variant<
        SqlToken,
        BinaryExpr,
        SelectStatement,
        InsertStatement,
        UpdateStatement,
        DeleteStatement,
        CreateTableStatement,
        CreateDbStatement,
        CreateSchemaStatement,
        ColumnDefinition>;

    struct AstNode
    {
        AstNodeType type;
        AstNodeValue value;

        AstNode() = default;

        AstNode(AstNodeType type, AstNodeValue&& value);
    };

    class SqlParser
    {
        std::vector<SqlToken> tokens_;
        size_t current_;

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

        CreateSchemaStatement
        parse_create_schema();

        BinaryExpr
        parse_binary(int min_priority);

        template <typename TEnum>
        bool
        match(const TEnum&) const;

        template <typename TEnum>
        void
        match_or_throw(TEnum expected, std::string error_msg = "Invalid syntax") const;

        bool
        advance() noexcept;

        bool
        advance_or_throw(std::string error_msg = "Invalid statement syntax");

        [[nodiscard]] const SqlToken&
        previous() const noexcept;

        [[nodiscard]] const SqlToken&
        current() const;

        std::unique_ptr<AstNode>
        parse_primary();

        std::vector<std::unique_ptr<AstNode> >
        parse_assignments();

        std::vector<std::unique_ptr<AstNode> >
        parse_tokens_list(SqlTokenType tokenType, AstNodeType nodeType);

        ColumnDefinition
        parse_column_def();

        TableIdentifier
        parse_table_identifier();

    public:
        SqlParser(std::vector<SqlToken> tokens);

        AstNode
        parse();
    };
} // namespace sql