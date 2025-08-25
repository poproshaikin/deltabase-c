#include "engine.hpp"
#include <chrono>
#include "sql/include/lexer.hpp"
#include "sql/include/parser.hpp"
#include "executor/include/semantic_analyzer.hpp"
#include "executor/include/query_executor.hpp"
#include <iostream>

DltEngine::DltEngine(std::string db_name) : db_name(db_name) { }

ExecutionResult DltEngine::run(const std::string& sql) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<sql::SqlToken> tokens;

    try {
        sql::SqlTokenizer tokenizer;
        tokens = tokenizer.tokenize(sql);
    } catch (...) {
        std::cout << "Execution failed at the tokenization phase" << std::endl;
        throw;
    }
    std::unique_ptr<sql::AstNode> node;

    try {
        sql::SqlParser parser(tokens);
        node = parser.parse();
    } catch (...) {
        std::cout << "Execution failed at the AST building phase" << std::endl;
        throw;
    }

    try {
        exe::SemanticAnalyzer analyzer(db_name);
        analyzer.analyze(node.get());
    } catch (...) {
        std::cout << "Execution failed at the analyzation phase" << std::endl;
        throw;
    }

    std::variant<std::unique_ptr<DataTable>, int> result;
    try {
        exe::QueryExecutor executor(db_name);
        result = executor.execute(*node);
    } catch (...) {
        std::cout << "Execution failed at the execution phase" << std::endl;
        throw;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return ExecutionResult(std::move(result), duration_ms);
}