#include "include/engine.hpp"
#include "../executor/include/executor.hpp"
#include "../executor/include/semantic_analyzer.hpp"
#include "../sql/include/lexer.hpp"
#include "../sql/include/parser.hpp"

#include <chrono>
#include <iostream>

namespace engine {
    DltEngine::DltEngine(EngineConfig cfg)
        : cfg_(cfg), registry_(cfg), semantic_analyzer_(registry_, cfg), 
          executor_(cfg, registry_), planner_(cfg, registry_) {
    }

    exe::QueryPlanExecutionResult
    DltEngine::execute(const sql::AstNode& node) {
        auto analysis = semantic_analyzer_.analyze(node);
        if (!analysis.is_valid) {
            std::cerr << "Execution failed at the analyzation phase" << std::endl;
            throw analysis.err.value();
        }
        
        return executor_.execute_plan(planner_.create_plan(node));
    }

    std::unique_ptr<sql::AstNode>
    DltEngine::build_ast(const std::string& sql) {
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

        return node;
    }

    ExecutionResult
    DltEngine::run_query(const std::string& sql) {
        auto start = std::chrono::high_resolution_clock::now();

        auto tree = build_ast(sql);
        auto result = execute(*tree);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        return ExecutionResult(std::move(result), duration_ns);
    }
} // namespace engine
