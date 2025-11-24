//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_QUERY_PLAN_HPP
#define DELTABASE_QUERY_PLAN_HPP
#include "ast_tree.hpp"
#include "data_row.hpp"

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
            UPDATE,
            DELETE
        };

        virtual Type
        type() const = 0;
    };

    struct QueryPlan
    {
        bool needs_stream = false;

        enum class Type
        {
            UNDEFINED = 0,
            SELECT,
            INSERT,
            UPDATE,
            DELETE
        };

        Type type = Type::UNDEFINED;
        std::unique_ptr<IPlanNode> root;
    };

    struct UnaryPlanNode : public IPlanNode
    {
        std::unique_ptr<IPlanNode> child;

        explicit
        UnaryPlanNode(std::unique_ptr<IPlanNode> child)
            : child(std::move(child))
        {
        };
    };

    struct LeafPlanNode : public IPlanNode
    {
    };

    struct SeqScanPlanNode final : public LeafPlanNode
    {
        std::string table_name;
        std::string schema_name;

        explicit
        SeqScanPlanNode(std::string table, std::string schema)
            : table_name(std::move(table)),
              schema_name(std::move(schema))
        {
        }

        Type
        type() const override
        {
            return Type::SEQ_SCAN;
        }
    };

    struct ValuesPlanNode final : LeafPlanNode
    {
        std::vector<DataRow> values;

        explicit
        ValuesPlanNode(std::vector<DataRow>&& values) : values(std::move(values))
        {
        }

        Type
        type() const override
        {
            return Type::VALUES;
        }
    };

    struct FilterPlanNode final : UnaryPlanNode
    {
        BinaryExpr where;
        MetaTable table;

        explicit
        FilterPlanNode(const MetaTable& table, BinaryExpr expr, std::unique_ptr<IPlanNode> child)
            : UnaryPlanNode(std::move(child)),
              where(std::move(expr)),
              table(table)
        {
        }

        Type
        type() const override
        {
            return Type::FILTER;
        }
    };

    struct ProjectPlanNode final : UnaryPlanNode
    {
        const MetaTable table;
        std::vector<std::string> columns;

        explicit
        ProjectPlanNode(const MetaTable& table, std::vector<std::string> cols, std::unique_ptr<IPlanNode> child)
            : UnaryPlanNode(std::move(child)),
              table(table),
              columns(std::move(cols))
        {
        }

        Type
        type() const override
        {
            return Type::PROJECT;
        }
    };

    struct LimitPlanNode final : UnaryPlanNode
    {
        uint64_t limit;

        explicit
        LimitPlanNode(uint64_t limit, std::unique_ptr<IPlanNode> child)
            : UnaryPlanNode(std::move(child)),
              limit(limit)
        {
        }

        Type
        type() const override
        {
            return Type::LIMIT;
        }
    };

    struct InsertPlanNode final : UnaryPlanNode
    {
        std::string table_name;
        std::string schema_name;
        std::optional<std::vector<std::string> > column_names;

        explicit
        InsertPlanNode(std::string table,
                       std::string schema,
                       std::optional<std::vector<std::string> > cols,
                       std::unique_ptr<IPlanNode> child)
            : UnaryPlanNode(std::move(child)),
              table_name(std::move(table)),
              schema_name(std::move(schema)),
              column_names(std::move(cols))
        {
        }

        Type
        type() const override
        {
            return Type::INSERT;
        }
    };

    struct UpdatePlanNode final : UnaryPlanNode
    {
        std::vector<Assignment> assignments;

        explicit
        UpdatePlanNode(std::vector<Assignment> asg, std::unique_ptr<IPlanNode> child)
            : UnaryPlanNode(std::move(child)),
              assignments(std::move(asg))
        {
        }

        Type
        type() const override
        {
            return Type::UPDATE;
        }
    };

    struct DeletePlanNode final : UnaryPlanNode
    {
        explicit
        DeletePlanNode(std::unique_ptr<IPlanNode> child)
            : UnaryPlanNode(std::move(child))
        {
        }

        Type
        type() const override
        {
            return Type::DELETE;
        }
    };
}

#endif //DELTABASE_QUERY_PLAN_HPP