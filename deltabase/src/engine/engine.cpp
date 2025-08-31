#include "include/engine.hpp"
#include "../executor/include/query_executor.hpp"
#include "../executor/include/semantic_analyzer.hpp"
#include "../sql/include/lexer.hpp"
#include "../sql/include/parser.hpp"
#include <chrono>
#include <iostream>

namespace engine {

    DltEngine::DltEngine() : registry(), router(this->registry), semantic_analyzer(this->registry) {
    }

    DltEngine::DltEngine(std::string db_name)
        : db_name(db_name), registry(), router(this->registry, db_name),
          semantic_analyzer(this->registry, db_name) {
    }

    exe::IntOrDataTable
    DltEngine::execute(const sql::AstNode& node) {
        exe::AnalysisResult analysis_result = this->semantic_analyzer.analyze(node);
        if (!analysis_result.is_valid) {
            std::cerr << "Execution failed at the analyzation phase" << std::endl;
            throw analysis_result.err.value();
        }

        try {
            return this->router.route_and_execute(node, analysis_result);
        } catch (...) {
            std::cerr << "Execution failed at the execution phase" << std::endl;
            throw;
        }
    }

    ExecutionResult
    DltEngine::run_query(const std::string& sql) {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<sql::SqlToken> tokens;

        try {
            sql::SqlTokenizer tokenizer;
            tokens = tokenizer.tokenize(sql);
        } catch (...) {
            std::cerr << "Execution failed at the tokenization phase" << std::endl;
            throw;
        }
        std::unique_ptr<sql::AstNode> node;

        try {
            sql::SqlParser parser(tokens);
            node = parser.parse();
        } catch (...) {
            std::cerr << "Execution failed at the AST building phase" << std::endl;
            throw;
        }

        exe::IntOrDataTable result = execute(*node);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        return ExecutionResult(std::move(result), duration_ns);
    }
} // namespace engine
