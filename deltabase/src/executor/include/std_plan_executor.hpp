//
// Created by poproshaikin on 10.11.25.
//

#ifndef DELTABASE_STD_PLAN_EXECUTOR_HPP
#define DELTABASE_STD_PLAN_EXECUTOR_HPP
#include "node_executor.hpp"
#include "plan_executor.hpp"
#include "../../storage/include/storage.hpp"

namespace exq
{
    class StdPlanExecutor final : public IPlanExecutor
    {
        storage::IStorage& storage_;
        NodeExecutorFactory node_executor_factory_;

        std::unique_ptr<types::IExecutionResult>
        execute_select(types::QueryPlan&& plan);

        std::unique_ptr<types::IExecutionResult>
        execute_insert(types::QueryPlan&& plan);

    public:
        explicit
        StdPlanExecutor(storage::IStorage& storage);

        std::unique_ptr<types::IExecutionResult>
        execute(types::QueryPlan&& plan) override;
    };

}

#endif //DELTABASE_STD_PLAN_EXECUTOR_HPP