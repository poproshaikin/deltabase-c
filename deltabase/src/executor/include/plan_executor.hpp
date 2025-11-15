//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_PLAN_EXECUTOR_HPP
#define DELTABASE_PLAN_EXECUTOR_HPP
#include "../../types/include/execution_result.hpp"
#include "../../types/include/query_plan.hpp"

namespace exq
{
    class IPlanExecutor
    {
    public:
        virtual ~IPlanExecutor() = default;

        virtual std::unique_ptr<types::IExecutionResult>
        execute(const types::QueryPlan& plan) = 0;
    };
}

#endif //DELTABASE_PLAN_EXECUTOR_HPP