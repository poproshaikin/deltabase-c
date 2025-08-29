#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "executor/include/query_executor.hpp"
#include "executor/include/semantic_analyzer.hpp"
#include <memory>
#include <string>
#include <vector>

struct ExecutionResult {
    exe::IntOrDataTable result;
    long execution_time_ms;

    ExecutionResult(exe::IntOrDataTable&& result, long execution_time_ms)
        : result(std::move(result)), execution_time_ms(execution_time_ms) {
    }
};

class DltEngine {

    std::vector<std::unique_ptr<exe::IQueryExecutor>> executors;

    exe::SemanticAnalyzer semantic_analyzer;

    meta::MetaRegistry registry;

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

#endif