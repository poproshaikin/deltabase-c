#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "../../executor/include/query_executor.hpp"
#include "../../executor/include/semantic_analyzer.hpp"
#include "query_router.hpp"
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
        QueryRouter router;
        exe::SemanticAnalyzer semantic_analyzer;
        catalog::MetaRegistry registry;

        exe::IntOrDataTable
        execute(const sql::AstNode& node);

    public:
        std::string db_name;

        DltEngine();
        DltEngine(std::string db_name);

        ExecutionResult
        run_query(const std::string& sql);
    };
} // namespace engine

#endif