//
// Created by poproshaikin on 15.11.25.
//

#include "std_plan_executor.hpp"

#include <format>

namespace exq
{
    using namespace types;

    std::unique_ptr<IExecutionResult>
    StdPlanExecutor::execute(const QueryPlan& plan)
    {
        try
        {
            if (plan.type == QueryPlanType::SELECT)
            {
                return execute_select(plan);
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
    StdPlanExecutor::execute_select(const QueryPlan& plan)
    {

    }
}