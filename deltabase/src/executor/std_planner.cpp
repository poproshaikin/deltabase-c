//
// Created by poproshaikin on 09.11.25.
//

#include "include/std_planner.hpp"
#include "../misc/include/convert.hpp"
#include "cli.hpp"

#include <format>
#include <functional>

namespace exq
{
    using namespace types;

    StdPlanner::StdPlanner(const DbConfig& db_config, storage::IStorage& storage)
        : storage_(storage), db_config_(db_config)
    {
    }

    QueryPlan
    StdPlanner::plan(AstNode&& ast)
    {
        try
        {
            if (ast.type == AstNodeType::SELECT)
            {
                return plan_select(std::get<SelectStatement>(ast.value));
            }
            if (ast.type == AstNodeType::INSERT)
            {
                return plan_insert(std::get<InsertStatement>(ast.value));
            }
            if (ast.type == AstNodeType::UPDATE)
            {
                return plan_update(std::get<UpdateStatement>(ast.value));
            }
            if (ast.type == AstNodeType::DELETE)
            {
                return plan_delete(std::get<DeleteStatement>(ast.value));
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

    QueryPlan
    StdPlanner::plan_select(SelectStatement& stmt) const
    {
        // 1. SEQ SCAN
        auto scan = std::make_unique<SeqScanPlanNode>(
            stmt.table.table_name.value,
            stmt.table.schema_name.has_value()
                ? stmt.table.schema_name.value().value
                : db_config_.default_schema
        );

        std::unique_ptr<IPlanNode> node = std::move(scan);

        // 2. WHERE
        if (stmt.where)
        {
            auto filter = std::make_unique<FilterPlanNode>(
                storage_.get_table(stmt.table),
                std::move(*stmt.where),
                std::move(node)
            );
            node = std::move(filter);
        }

        // 3. PROJECT
        if (!stmt.columns.empty())
        {
            std::vector<std::string> cols;
            for (auto& c : stmt.columns)
                cols.push_back(c.value);

            auto project = std::make_unique<ProjectPlanNode>(
                storage_.get_table(stmt.table),
                cols,
                std::move(node)
            );
            node = std::move(project);
        }

        // 4. LIMIT
        if (stmt.limit)
        {
            auto limit = std::make_unique<LimitPlanNode>(stmt.limit.value(), std::move(node));
            node = std::move(limit);
        }

        QueryPlan plan;
        plan.type = QueryPlan::Type::SELECT;
        plan.needs_stream = storage_.needs_stream(*node);
        plan.root = std::move(node);
        return plan;
    }

    QueryPlan
    StdPlanner::plan_insert(InsertStatement& stmt) const
    {
        std::vector<DataRow> rows;
        for (auto& [vals] : stmt.values)
            rows.emplace_back(vals);

        auto values_node = std::make_unique<ValuesPlanNode>(std::move(rows));

        std::optional<std::vector<std::string> > cols{};
        if (!stmt.columns.empty())
        {
            std::vector<std::string> col_names;
            for (const auto& col : stmt.columns)
                col_names.push_back(col.value);
            cols.emplace(std::move(col_names));
        }

        auto insert = std::make_unique<InsertPlanNode>(
            stmt.table.table_name.value,
            stmt.table.schema_name.has_value()
                ? stmt.table.schema_name.value().value
                : db_config_.default_schema,
            cols,
            std::move(values_node)
        );

        QueryPlan plan;
        plan.root = std::move(insert);
        plan.type = QueryPlan::Type::INSERT;
        plan.needs_stream = false;

        return plan;
    }

    QueryPlan
    StdPlanner::plan_update(UpdateStatement& stmt) const
    {
        const auto table = storage_.get_table(stmt.table);

        std::vector<Assignment> assignments;
        for (const auto& assignment : stmt.assignments)
        {
            if (assignment.left->type == AstNodeType::COLUMN_IDENTIFIER &&
                assignment.right->type == AstNodeType::LITERAL)
            {
                const auto& col_id = table.get_column(stmt.table.table_name).id;
                auto data_token = misc::convert(std::get<SqlToken>(assignment.right->value));

                assignments.emplace_back(std::make_pair(col_id, data_token));
            }
            if (assignment.left->type == AstNodeType::COLUMN_IDENTIFIER &&
                assignment.right->type == AstNodeType::COLUMN_IDENTIFIER)
            {
                const auto& left = table.get_column(std::get<SqlToken>(assignment.left->value));
                const auto& right = table.get_column(std::get<SqlToken>(assignment.right->value));

                assignments.emplace_back(std::make_pair(left.id, right.id));
            }
        }

        std::unique_ptr<IPlanNode> root =
            std::make_unique<SeqScanPlanNode>(
                stmt.table.table_name,
                stmt.table.schema_name.has_value()
                    ? stmt.table.schema_name.value().value
                    : db_config_.default_schema
            );

        if (stmt.where)
        {
            root = std::make_unique<FilterPlanNode>(
                storage_.get_table(stmt.table),
                std::move(*stmt.where),
                std::move(root));
        }

        auto update = std::make_unique<UpdatePlanNode>(
            std::move(assignments),
            std::move(root));

        QueryPlan plan;
        plan.root = std::move(update);
        plan.type = QueryPlan::Type::UPDATE;
        plan.needs_stream = false;

        return plan;
    }

    QueryPlan
    StdPlanner::plan_delete(DeleteStatement& stmt) const
    {
        std::unique_ptr<IPlanNode> root =
            std::make_unique<SeqScanPlanNode>(
                stmt.table.table_name,
                stmt.table.schema_name.has_value()
                    ? stmt.table.schema_name.value().value
                    : db_config_.default_schema
            );

        if (stmt.where)
        {
            root = std::make_unique<FilterPlanNode>(
                storage_.get_table(stmt.table),
                std::move(*stmt.where),
                std::move(root));
        }

        auto del = std::make_unique<DeletePlanNode>(std::move(root));

        QueryPlan plan;
        plan.root = std::move(del);
        plan.type = QueryPlan::Type::DELETE;
        plan.needs_stream = false;

        return plan;
    }
}