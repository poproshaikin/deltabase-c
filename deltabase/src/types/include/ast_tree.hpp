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

        BinaryExpr() = default;
        BinaryExpr(BinaryExpr&&) = default;
        BinaryExpr& operator=(BinaryExpr&&) = default;
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
}


#endif //DELTABASE_AST_TREE_HPP