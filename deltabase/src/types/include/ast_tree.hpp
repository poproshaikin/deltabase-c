//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_AST_TREE_HPP
#define DELTABASE_AST_TREE_HPP
#include "sql_token.hpp"
#include "config.hpp"

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
        ADD_COLUMN,
        BEGIN,
        COMMIT,
        ROLLBACK
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

        BinaryExpr(const BinaryExpr&);
        BinaryExpr& operator=(const BinaryExpr&);

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

        std::string
        require_schema(const Config& cfg) const
        {
            return schema_name.has_value() ? schema_name.value().value : cfg.default_schema;
        }
    };

    struct SelectStmt
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

    struct InsertStmt
    {
        TableIdentifier table;
        std::vector<SqlToken> columns;
        std::vector<ValuesExpr> values;
    };

    struct UpdateStmt
    {
        TableIdentifier table;
        std::vector<BinaryExpr> assignments;
        std::optional<BinaryExpr> where;
    };

    struct DeleteStmt
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

    enum class OnDeleteFkAction
    {
        RESTRICT,
        CASCADE,
        SET_NULL
    };

    struct ForeignKeyConstraint
    {
        TableIdentifier referenced_table;
        SqlToken referenced_column;
        OnDeleteFkAction action;
    };

    struct AutoIncrementConstraint
    {
    };

    struct DefaultConstraint
    {
        SqlToken value;

        explicit
        DefaultConstraint(SqlToken value = SqlToken())
            : value(std::move(value))
        {
        }
    };

    using Constraint = std::variant<
        NotNullConstraint,
        PrimaryKeyConstraint,
        ForeignKeyConstraint,
        AutoIncrementConstraint,
        DefaultConstraint
    >;

    struct ColumnDefinition
    {
        SqlToken name;
        SqlToken type;
        std::vector<Constraint> constraints;
    };

    struct CreateTableStmt
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

    struct AlterTableStmt
    {
        TableIdentifier table;
        std::vector<AlterTableOperation> operations;
    };

    struct CreateSchemaStmt
    {
        SqlToken name;
    };

    struct CreateIndexStmt
    {
        TableIdentifier table;
        SqlToken index_name;
        SqlToken column_name;
        bool is_unique;
    };

    struct DropIndexStmt
    {
        TableIdentifier table;
        SqlToken index_name;
    };

    struct DropTableStmt
    {
        TableIdentifier table;
    };

    struct CreateDatabaseStmt
    {
        SqlToken name;
    };

    struct BeginTxnStmt
    {
    };

    struct CommitTxnStmt
    {
    };

    struct RollbackTxnStmt
    {
    };

    using AstNodeValue = std::variant<
        SqlToken,
        BinaryExpr,
        SelectStmt,
        InsertStmt,
        UpdateStmt,
        DeleteStmt,
        CreateTableStmt,
        CreateDatabaseStmt,
        CreateSchemaStmt,
        CreateIndexStmt,
        DropIndexStmt,
        DropTableStmt,
        ColumnDefinition,
        AlterTableStmt,
        BeginTxnStmt,
        CommitTxnStmt,
        RollbackTxnStmt
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