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
        QueryPlanNode node{
            SeqScanPlanNode{
                .table_name = stmt.table.table_name.value,
                .schema_name = stmt.table.schema_name.has_value()
                                   ? stmt.table.schema_name.value().value
                                   : db_config_.default_schema
            }
        };

        if (stmt.where)
            node = QueryPlanNode{
                FilterPlanNode{std::move(*stmt.where),
                               std::make_unique<QueryPlanNode>(std::move(node))}};

        if (!stmt.columns.empty())
        {
            ProjectPlanNode project;
            for (const auto& column : stmt.columns)
                project.columns.push_back(column.value);
            project.child = std::make_unique<QueryPlanNode>(std::move(node));
            node = QueryPlanNode{std::move(project)};
        }

        if (stmt.limit)
            node = QueryPlanNode{
                LimitPlanNode{stmt.limit.value(),
                              std::make_unique<QueryPlanNode>(std::move(node))}
            };

        QueryPlan plan{
            .needs_stream = storage_.needs_stream(node),
            .type = QueryPlanType::SELECT,
            .node = std::move(node),
        };

        return plan;
    }

    QueryPlan
    StdPlanner::plan_insert(InsertStatement& stmt) const
    {
        InsertPlanNode insert{
            .table_name = stmt.table.table_name.value,
            .schema_name = stmt.table.schema_name.has_value()
                               ? stmt.table.schema_name.value().value
                               : db_config_.default_schema,
        };

        if (!stmt.columns.empty())
        {
            for (auto col : stmt.columns)
            {
                insert.column_names.emplace();
                insert.column_names->push_back(col);
            }
        }

        // TODO: temporary, extend when implement sub-SELECT functionality
        ValuesPlanNode values_node;
        for (const auto& [values] : stmt.values)
            values_node.values.emplace_back(DataRow(values));

        insert.child = std::make_unique<QueryPlanNode>(std::move(values_node));
        QueryPlan plan{
            .needs_stream = false,
            .type = QueryPlanType::INSERT,
            .node = std::move(insert),
        };
        return plan;
    }

    QueryPlan
    StdPlanner::plan_update(UpdateStatement& stmt) const
    {
        auto table = storage_.get_table(stmt.table);

        UpdatePlanNode update{};
        for (const auto& assignment : stmt.assignments)
        {
            if (assignment.left->type == AstNodeType::COLUMN_IDENTIFIER &&
                assignment.right->type == AstNodeType::LITERAL)
            {
                const auto& col_id = table->get_column(stmt.table.table_name).id;
                auto data_token = misc::convert(std::get<SqlToken>(assignment.right->value));

                update.assignments.emplace_back(std::make_pair(col_id, data_token));
            }
            if (assignment.left->type == AstNodeType::COLUMN_IDENTIFIER &&
                assignment.right->type == AstNodeType::COLUMN_IDENTIFIER)
            {
                const auto& left = table->get_column(std::get<SqlToken>(assignment.left->value));
                const auto& right = table->get_column(std::get<SqlToken>(assignment.right->value));

                update.assignments.emplace_back(std::make_pair(left.id, right.id));
            }
        }

        QueryPlanNode plan_node{
            SeqScanPlanNode{
                .table_name = stmt.table.table_name,
                .schema_name = stmt.table.schema_name.has_value()
                                   ? stmt.table.schema_name.value().value
                                   : db_config_.default_schema,
            }
        };

        if (stmt.where)
        {
            FilterPlanNode filter_node{
                .where = std::move(stmt.where.value()),
                .child = std::make_unique<QueryPlanNode>(std::move(plan_node))
            };
            plan_node = std::move(filter_node);
        }

        update.child = std::make_unique<QueryPlanNode>(std::move(plan_node));

        QueryPlan plan{
            .needs_stream = false,
            .type = QueryPlanType::UPDATE,
            .node = std::move(plan_node),
        };

        return plan;
    }

    QueryPlan
    StdPlanner::plan_delete(DeleteStatement& stmt) const
    {
        QueryPlanNode plan_node{
            SeqScanPlanNode{
                .table_name = stmt.table.table_name,
                .schema_name = stmt.table.schema_name.has_value()
                                   ? stmt.table.schema_name.value().value
                                   : db_config_.default_schema,
            }
        };

        if (stmt.where)
        {
            FilterPlanNode filter_node{
                .where = std::move(stmt.where.value()),
                .child = std::make_unique<QueryPlanNode>(std::move(plan_node))
            };
            plan_node = std::move(filter_node);
        }

        DeletePlanNode delete_node{
            .child = std::make_unique<QueryPlanNode>(std::move(plan_node))
        };

        QueryPlan plan{
            .needs_stream = false,
            .type = QueryPlanType::DELETE,
            .node = std::move(plan_node)
        };

        return plan;
    }
}