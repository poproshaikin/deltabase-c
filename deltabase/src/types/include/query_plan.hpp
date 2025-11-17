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
            From,
            Filter,
            Project,
            Limit,
            Insert,
            Values,
            SeqScan,
            Update,
            Delete
        };

        virtual Type
        type() const = 0;
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
            return Type::SeqScan;
        }
    };

    struct FromPlanNode final : UnaryPlanNode
    {
        std::string table_name;
        std::string schema_name;

        explicit
        FromPlanNode(std::string table, std::string schema, std::unique_ptr<IPlanNode> child)
            : UnaryPlanNode(std::move(child)),
              table_name(std::move(table)),
              schema_name(std::move(schema))
        {
        }

        Type
        type() const override
        {
            return Type::From;
        }
    };

    struct FilterPlanNode final : UnaryPlanNode
    {
        BinaryExpr where;

        explicit
        FilterPlanNode(BinaryExpr expr, std::unique_ptr<IPlanNode> child)
            : UnaryPlanNode(std::move(child)),
              where(std::move(expr))
        {
        }

        Type
        type() const override
        {
            return Type::Filter;
        }
    };

    struct ProjectPlanNode final : UnaryPlanNode
    {
        std::vector<std::string> columns;

        explicit
        ProjectPlanNode(std::vector<std::string> cols, std::unique_ptr<IPlanNode> child)
            : UnaryPlanNode(std::move(child)),
              columns(std::move(cols))
        {
        }

        Type
        type() const override
        {
            return Type::Project;
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
            return Type::Limit;
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
            return Type::Insert;
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
            return Type::Update;
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
            return Type::Delete;
        }
    };


}

#endif //DELTABASE_QUERY_PLAN_HPP