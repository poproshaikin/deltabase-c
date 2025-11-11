//
// Created by poproshaikin on 09.11.25.
//

#include "include/std_planner.hpp"

#include <format>

namespace exq
{
    using namespace types;

    QueryPlanNode
    StdPlanner::plan(AstNode&& ast)
    {
        try
        {
            if (ast.type == AstNodeType::SELECT)
            {
                return plan_select(std::get<SelectStatement>(ast.value));
            }
        }
        catch (std::exception ex)
        {
            throw std::runtime_error("StdPlanner::plan: " + std::string(ex.what()));
        }

        throw std::runtime_error(
            std::format("Unsupported query in StdPlanner::plan: {}",
                        static_cast<std::underlying_type_t<decltype(ast.type)>>
                        (ast.type)
            )
        );
    }

    QueryPlanNode
    StdPlanner::plan_select(SelectStatement& stmt) const
    {
        QueryPlanNode plan{SeqScanPlanNode{
            stmt.table.table_name.value,
            stmt.table.schema_name.has_value()
                ? std::optional(stmt.table.schema_name.value().value)
                : std::nullopt
        }};

        if (stmt.where)
            plan = QueryPlanNode{
                FilterPlanNode{std::move(*stmt.where),
                               std::make_unique<QueryPlanNode>(std::move(plan))}};

        if (!stmt.columns.empty())
        {
            ProjectPlanNode project;
            for (const auto& column : stmt.columns)
                project.columns.push_back(column.value);
            project.child = std::make_unique<QueryPlanNode>(std::move(plan));
            plan = QueryPlanNode{std::move(project)};
        }

        if (stmt.limit)
            plan = QueryPlanNode{
                LimitPlanNode{stmt.limit.value(),
                              std::make_unique<QueryPlanNode>(std::move(plan))}};

        return plan;
    }
}