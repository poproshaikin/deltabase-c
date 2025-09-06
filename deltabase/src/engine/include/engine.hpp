#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "../../executor/include/semantic_analyzer.hpp"
#include "../../executor/include/action.hpp"
#include "../../executor/include/executor.hpp"
#include "../../executor/include/planner.hpp"
#include "config.hpp"

#include <string>

namespace engine {

    struct ExecutionResult {

        exe::IntOrDataTable result;
        std::pair<std::string, exe::ActionError> error;
        long execution_time_ns;

        ExecutionResult(exe::QueryPlanExecutionResult&& result, long execution_time_ns);
        ExecutionResult(std::pair<std::string, exe::ActionError> error);
        ExecutionResult(exe::IntOrDataTable&& result, long execution_time_ns);
    };

    class DltEngine {
        exe::SemanticAnalyzer semantic_analyzer_;
        exe::QueryPlanner planner_;
        exe::ActionExecutor executor_;
        catalog::MetaRegistry registry_;
        EngineConfig cfg_;

        exe::QueryPlanExecutionResult
        execute(sql::AstNode& node);

        std::unique_ptr<sql::AstNode>
        build_ast(const std::string& sql);

    public:
        DltEngine(EngineConfig cfg_ = {});

        auto
        run_query(const std::string& sql) -> ExecutionResult;
    };
} // namespace engine

#endif