//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_ENGINE_HPP
#define DELTABASE_ENGINE_HPP

#include "planner_factory.hpp"
#include "semantic_analyzer.hpp"
#include "../../sql/include/parser.hpp"
#include "../../types/include/execution_result.hpp"
#include "../../storage/include/db_instance.hpp"
#include "../../types/include/config.hpp"
#include "../../executor/include/node_executor.hpp"
#include "../../executor/include/planner.hpp"

namespace engine
{
    class Engine
    {
        sql::SqlParser parser_;
        std::unique_ptr<exq::SemanticAnalyzer> analyzer_;
        std::unique_ptr<exq::IPlanner> planner_;
        std::unique_ptr<storage::IDbInstance> db_;
        exq::NodeExecutorFactory executor_factory_;
        exq::PlannerFactory planner_factory_;

        types::Config
        load_config(const std::string& name, const std::filesystem::path& current_path) const;

        void
        init_db(std::unique_ptr<storage::IDbInstance> db);

        std::unique_ptr<types::IExecutionResult>
        execute_generic(types::QueryPlan&& plan);

    public:
        explicit
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