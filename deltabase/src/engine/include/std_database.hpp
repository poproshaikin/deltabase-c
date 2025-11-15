//
// Created by poproshaikin on 10.11.25.
//

#ifndef DELTABASE_STD_DATABASE_HPP
#define DELTABASE_STD_DATABASE_HPP
#include "database.hpp"
#include "plan_executor.hpp"
#include "planner.hpp"
#include "storage.hpp"
#include "../../types/include/db_cfg.hpp"

namespace engine
{
    class StdDatabase final : public IDatabase
    {
        std::string db_name_;
        types::DbConfig db_config_;
        std::unique_ptr<storage::IStorage> storage_;
        std::unique_ptr<exq::IPlanner> planner_;
        std::unique_ptr<exq::IPlanExecutor> plan_executor_;

    public:
        explicit
        StdDatabase(const std::string& db_name, const types::DbConfig& db_config);

        std::unique_ptr<types::IExecutionResult>
        execute(const std::string& query) override;
    };
}

#endif //DELTABASE_STD_DATABASE_HPP