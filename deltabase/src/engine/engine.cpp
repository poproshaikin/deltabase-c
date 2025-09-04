#include "include/engine.hpp"
#include "../executor/include/semantic_analyzer.hpp"
#include "../sql/include/lexer.hpp"
#include "../sql/include/parser.hpp"
#include <chrono>
#include <iostream>

namespace engine {
    DltEngine::DltEngine(EngineConfig cfg)
        : cfg_(cfg), registry_(), router_(registry_, cfg),
          semantic_analyzer_(registry_, cfg) {
    }

    auto
    DltEngine::execute(const sql::AstNode& node) -> exe::IntOrDataTable {
        exe::AnalysisResult analysis_result = semantic_analyzer_.analyze(node);
        if (!analysis_result.is_valid) {
            std::cerr << "Execution failed at the analyzation phase" << std::endl;
            throw analysis_result.err.value();
        }

        return router_.route_and_execute(node, analysis_result);
    }

    auto
    DltEngine::run_query(const std::string& sql) -> ExecutionResult {
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

        return {std::move(result), duration_ns};
    }
} // namespace engine
