#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "../../executor/include/query_executor.hpp"
#include "../../executor/include/semantic_analyzer.hpp"
#include <memory>
#include <string>
#include <vector>

namespace engine {

    struct ExecutionResult {

        exe::IntOrDataTable result;
        long execution_time_ns;

        ExecutionResult(exe::IntOrDataTable&& result, long execution_time_ns)
            : result(std::move(result)), execution_time_ns(execution_time_ns) {
        }
    };

    class DltEngine {

        std::vector<std::unique_ptr<exe::IQueryExecutor>> executors;

        exe::SemanticAnalyzer semantic_analyzer;

        catalog::MetaRegistry registry;

        exe::IsSupportedType
        can_execute(const sql::AstNodeType& type);

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