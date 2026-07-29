//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_ENGINE_HPP
#define DELTABASE_ENGINE_HPP

#include "execution_context.hpp"
#include <functional>
#include "planner_factory.hpp"
#include "semantic_analyzer.hpp"
#include "storage_service_provider.hpp"
#include "../../sql/include/parser.hpp"
#include "../../types/include/execution_result.hpp"
#include "../../types/include/config.hpp"
#include "../../executor/include/node_executor.hpp"
#include "../../executor/include/planner.hpp"
#include "../../types/include/query_plan.hpp"

namespace engine
{
    class Engine
    {
        sql::SqlParser parser_;
        std::unique_ptr<exq::SemanticAnalyzer> analyzer_;
        std::unique_ptr<exq::IPlanner> planner_;
        std::unique_ptr<storage::StorageServiceProvider> storage_service_provider_;
        exq::NodeExecutorFactory executor_factory_;
        exq::PlannerFactory planner_factory_;

        std::optional<txn::Transaction> active_txn_;
        types::ExecutionContext ctx_;

        types::Config
        load_config(const std::string& name, const std::filesystem::path& executable_path) const;

        void
        reset_storage(const types::Config& config);

        std::unique_ptr<types::IExecutionResult>
        make_ok_result(const std::string& tag);

        void
        commit_active_txn();

        std::unique_ptr<types::IExecutionResult>
        execute(bool needs_stream, types::IPlanNode& root, std::function<void()> on_done);

    public:
        Engine();

        void
        attach_db(const std::string& db_name);

        void
        create_db(const types::Config& config);

        void
        detach_db();

        std::unique_ptr<types::IExecutionResult>
        execute_query(const std::string& query);
    };
}

#endif //DELTABASE_ENGINE_HPP