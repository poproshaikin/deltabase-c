//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_AST_TREE_HPP
#define DELTABASE_AST_TREE_HPP
#include "sql_token.hpp"

#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace types
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
        CREATE_INDEX,
        DROP_INDEX,
        DROP_TABLE,
        ALTER_TABLE,
        ADD_COLUMN
    };

    enum class AstOperator
    {
        UNDEFINED = 0,
        OR = 1,
        AND,
        NOT,

        EQ,
        NEQ,

        GR,
        GRE,
        LT,
        LTE,

        IS,

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
            {AstOperator::IS, 4},
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

        BinaryExpr() = default;
        BinaryExpr(BinaryExpr&&) = default;
        BinaryExpr& operator=(BinaryExpr&&) = default;

        std::string
        to_string() const;
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
        std::optional<uint64_t> limit;
    };

    struct ValuesExpr
    {
        std::vector<SqlToken> values;
    };

    struct InsertStatement
    {
        TableIdentifier table;
        std::vector<SqlToken> columns;
        std::vector<ValuesExpr> values;
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

    struct NotNullConstraint
    {
    };

    struct PrimaryKeyConstraint
    {
    };

    struct DefaultConstraint
    {
        SqlToken value;

        explicit DefaultConstraint(SqlToken value = SqlToken())
            : value(std::move(value))
        {
        }
    };

    using Constraint = std::variant<
        NotNullConstraint,
        PrimaryKeyConstraint,
        DefaultConstraint
    >;

    struct ColumnDefinition
    {
        SqlToken name;
        SqlToken type;
        std::vector<Constraint> constraints;
    };

    struct CreateTableStatement
    {
        TableIdentifier table;
        std::vector<ColumnDefinition> columns;
    };

    struct AddColumnOperation
    {
        ColumnDefinition column;
    };

    using AlterTableOperation = std::variant<
        AddColumnOperation
    >;

    struct AlterTableStatement
    {
        TableIdentifier table;
        std::vector<AlterTableOperation> operations;
    };

    struct CreateSchemaStatement
    {
        SqlToken name;
    };

    struct CreateIndexStatement
    {
        TableIdentifier table;
        SqlToken index_name;
        SqlToken column_name;
        bool is_unique;
        bool is_primary;
    };

    struct DropIndexStatement
    {
        TableIdentifier table;
        SqlToken index_name;
    };

    struct DropTableStatement
    {
        TableIdentifier table;
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
        CreateIndexStatement,
        DropIndexStatement,
        DropTableStatement,
        ColumnDefinition,
        AlterTableStatement
    >;

    struct AstNode
    {
        AstNodeType type;
        AstNodeValue value;

        AstNode() = default;

        AstNode(AstNodeType type, AstNodeValue&& value);
    };
}


#endif //DELTABASE_AST_TREE_HPP