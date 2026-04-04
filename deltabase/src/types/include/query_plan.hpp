//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_QUERY_PLAN_HPP
#define DELTABASE_QUERY_PLAN_HPP
#include "ast_tree.hpp"
#include "data_row.hpp"
#include "meta_schema.hpp"
#include "meta_table.hpp"

#include <string>
#include <variant>

namespace types
{
    struct IPlanNode
    {
        virtual ~IPlanNode() = default;

        enum class Type
        {
            UNDEFINED = 0,
            FILTER,
            PROJECT,
            LIMIT,
            INSERT,
            VALUES,
            SEQ_SCAN,
            INDEX_SCAN,
            UPDATE,
            DELETE,
            CREATE_DB,
            CREATE_TABLE,
            CREATE_INDEX,
            DROP_INDEX
        };

        virtual constexpr Type
        type() const = 0;
    };

    struct QueryPlan
    {
        bool needs_stream;
        bool db_specific;

        enum class Type
        {
            UNDEFINED = 0,
            SELECT,
            INSERT,
            UPDATE,
            DELETE,
            CREATE_DB,
            CREATE_TABLE,
            CREATE_INDEX,
            DROP_INDEX
        };

        Type type = Type::UNDEFINED;
        std::unique_ptr<IPlanNode> root;
    };

    struct UnaryPlanNode : IPlanNode
    {
        std::unique_ptr<IPlanNode> child;

        explicit UnaryPlanNode(std::unique_ptr<IPlanNode> child) : child(std::move(child)) {};
    };

    struct LeafPlanNode : IPlanNode
    {
    };

    struct SeqScanPlanNode final : LeafPlanNode
    {
        std::string table_name;
        std::string schema_name;

        explicit SeqScanPlanNode(std::string table, std::string schema)
            : table_name(std::move(table)), schema_name(std::move(schema))
        {
        }

        constexpr Type
        type() const override
        {
            return Type::SEQ_SCAN;
        }
    };

    struct IndexScanPlanNode final : LeafPlanNode
    {
        std::string table_name;
        std::string schema_name;
        IndexId index_id;
        BinaryExpr condition;

        explicit IndexScanPlanNode(
            const std::string& table_name,
            const std::string& schema_name,
            const IndexId& index_id,
            BinaryExpr condition
        )
            : table_name(table_name), schema_name(schema_name), index_id(index_id),
              condition(std::move(condition))
        {
        }

        constexpr Type
        type() const override
        {
            return Type::INDEX_SCAN;
        }
    };

    struct ValuesPlanNode final : LeafPlanNode
    {
        std::vector<DataRow> values;

        explicit ValuesPlanNode(std::vector<DataRow>&& values) : values(std::move(values))
        {
        }

        constexpr Type
        type() const override
        {
            return Type::VALUES;
        }
    };

    struct FilterPlanNode final : UnaryPlanNode
    {
        BinaryExpr where;
        MetaTable table;

        explicit FilterPlanNode(
            const MetaTable& table, BinaryExpr expr, std::unique_ptr<IPlanNode> child
        )
            : UnaryPlanNode(std::move(child)), where(std::move(expr)), table(table)
        {
        }

        constexpr Type
        type() const override
        {
            return Type::FILTER;
        }
    };

    struct ProjectPlanNode final : UnaryPlanNode
    {
        const MetaTable table;
        std::vector<std::string> columns;

        explicit ProjectPlanNode(
            const MetaTable& table, std::vector<std::string> cols, std::unique_ptr<IPlanNode> child
        )
            : UnaryPlanNode(std::move(child)), table(table), columns(std::move(cols))
        {
        }

        constexpr Type
        type() const override
        {
            return Type::PROJECT;
        }
    };

    struct LimitPlanNode final : UnaryPlanNode
    {
        uint64_t limit;

        explicit LimitPlanNode(uint64_t limit, std::unique_ptr<IPlanNode> child)
            : UnaryPlanNode(std::move(child)), limit(limit)
        {
        }

        constexpr Type
        type() const override
        {
            return Type::LIMIT;
        }
    };

    struct InsertPlanNode final : UnaryPlanNode
    {
        std::string table_name;
        std::string schema_name;
        std::optional<std::vector<std::string>> column_names;

        explicit InsertPlanNode(
            std::string table,
            std::string schema,
            std::optional<std::vector<std::string>> cols,
            std::unique_ptr<IPlanNode> child
        )
            : UnaryPlanNode(std::move(child)), table_name(std::move(table)),
              schema_name(std::move(schema)), column_names(std::move(cols))
        {
        }

        constexpr Type
        type() const override
        {
            return Type::INSERT;
        }
    };

    struct UpdatePlanNode final : UnaryPlanNode
    {
        std::string table_name;
        std::string schema_name;
        std::vector<Assignment> assignments;

        explicit UpdatePlanNode(
            const std::string& table_name,
            const std::string& schema_name,
            const std::vector<Assignment>& asg,
            std::unique_ptr<IPlanNode> child
        )
            : UnaryPlanNode(std::move(child)), table_name(table_name), schema_name(schema_name),
              assignments(asg)
        {
        }

        constexpr Type
        type() const override
        {
            return Type::UPDATE;
        }
    };

    struct DeletePlanNode final : UnaryPlanNode
    {
        std::string table_name;
        std::string schema_name;

        explicit DeletePlanNode(
            const std::string& table_name,
            const std::string& schema_name,
            std::unique_ptr<IPlanNode> child
        )
            : UnaryPlanNode(std::move(child)), table_name(table_name), schema_name(schema_name)
        {
        }

        constexpr Type
        type() const override
        {
            return Type::DELETE;
        }
    };

    struct CreateDbPlanNode final : LeafPlanNode
    {
        std::string db_name;

        explicit CreateDbPlanNode(const std::string& db_name) : db_name(db_name)
        {
        }

        constexpr Type
        type() const override
        {
            return Type::CREATE_DB;
        }
    };

    struct CreateTablePlanNode final : LeafPlanNode
    {
        std::string table_name;
        MetaSchema schema;
        std::vector<ColumnDefinition> columns;

        explicit CreateTablePlanNode(
            const std::string& table_name,
            const MetaSchema& schema,
            const std::vector<ColumnDefinition>& columns
        )
            : table_name(table_name), schema(schema), columns(columns)
        {
        }

        constexpr Type
        type() const override
        {
            return Type::CREATE_TABLE;
        }
    };

    struct CreateIndexPlanNode final : LeafPlanNode
    {
        std::string index_name;
        std::string table_name;
        std::string schema_name;
        std::string column_name;
        bool is_unique;

        explicit CreateIndexPlanNode(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name,
            const std::string& column_name,
            bool is_unique
        )
            : index_name(index_name), table_name(table_name), schema_name(schema_name),
              column_name(column_name), is_unique(is_unique)
        {
        }

        constexpr Type
        type() const override
        {
            return Type::CREATE_INDEX;
        }
    };

    struct DropIndexPlanNode final : LeafPlanNode
    {
        std::string index_name;
        std::string table_name;
        std::string schema_name;

        explicit DropIndexPlanNode(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name
        )
            : index_name(index_name), table_name(table_name), schema_name(schema_name)
        {
        }

        constexpr Type
        type() const override
        {
            return Type::DROP_INDEX;
        }
    };
} // namespace types

#endif // DELTABASE_QUERY_PLAN_HPP