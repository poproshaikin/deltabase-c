//
// Created by poproshaikin on 15.11.25.
//

#include "std_plan_executor.hpp"

#include <format>

namespace exq
{
    using namespace types;

    StdPlanExecutor::StdPlanExecutor(storage::IStorage& storage)
        : storage_(storage), node_executor_factory_(storage_)
    {
    }

    std::unique_ptr<IExecutionResult>
    StdPlanExecutor::execute(QueryPlan&& plan)
    {
        try
        {
            if (plan.type == QueryPlan::Type::SELECT)
            {
                return execute_select(std::move(plan));
            }
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error("StdPlanner::plan: " + std::string(ex.what()));
        }

        throw std::runtime_error(
            std::format("Unsupported query in StdPlanExecutor::execute: {}",
                        static_cast<std::underlying_type_t<decltype(plan.type)>>
                        (plan.type)
            )
        );
    }

    std::unique_ptr<IExecutionResult>
    StdPlanExecutor::execute_select(QueryPlan&& plan)
    {
        // TODO vymyslet stream mode;

        auto root_exq = node_executor_factory_.from_plan(std::move(plan.root));

        root_exq->open();
        DataTable table;
        table.output_schema = root_exq->output_schema();

        while (true)
        {
            DataRow row;
            if (!root_exq->next(row))
                break;

            table.rows.push_back(std::move(row));
        }

        return std::make_unique<MaterializedResult>(table);
    }

    std::unique_ptr<IExecutionResult>
    StdPlanExecutor::execute_insert(QueryPlan&& plan)
    {
        auto root_exq = node_executor_factory_.from_plan(std::move(plan.root));

        root_exq->open();
        DataRow rows_affected;
        root_exq->next(rows_affected);
        root_exq->close();

        DataTable table;
        table.output_schema = root_exq->output_schema();
        table.rows.emplace_back(std::move(rows_affected));
        return std::make_unique<MaterializedResult>(table);
    }
}