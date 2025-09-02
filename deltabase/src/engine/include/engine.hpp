#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "../../executor/include/query_executor.hpp"
#include "../../executor/include/semantic_analyzer.hpp"
#include "query_router.hpp"
#include "config.hpp"
#include <string>

namespace engine {

    struct ExecutionResult {

        exe::IntOrDataTable result;
        long execution_time_ns;

        ExecutionResult(exe::IntOrDataTable&& result, long execution_time_ns)
            : result(std::move(result)), execution_time_ns(execution_time_ns) {
        }
    };

    class DltEngine {
        QueryRouter router_;
        exe::SemanticAnalyzer semantic_analyzer_;
        catalog::MetaRegistry registry_;
        EngineConfig cfg_;

        auto
        execute(const sql::AstNode& node) -> exe::IntOrDataTable;

    public:
        DltEngine(EngineConfig cfg_ = {});

        auto
        run_query(const std::string& sql) -> ExecutionResult;
    };
} // namespace engine

#endif