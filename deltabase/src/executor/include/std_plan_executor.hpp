//
// Created by poproshaikin on 10.11.25.
//

#ifndef DELTABASE_SIMPLE_PLAN_EXECUTOR_HPP
#define DELTABASE_SIMPLE_PLAN_EXECUTOR_HPP
#include "plan_executor.hpp"

namespace exq
{
    class StdPlanExecutor final : public IPlanExecutor
    {
        std::unique_ptr<types::IExecutionResult>
        execute(const types::QueryPlan& plan) override;
    };
}

#endif //DELTABASE_SIMPLE_PLAN_EXECUTOR_HPP