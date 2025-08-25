#include "engine.hpp"
#include <chrono>
#include "sql/include/lexer.hpp"
#include "sql/include/parser.hpp"
#include "executor/include/semantic_analyzer.hpp"
#include "executor/include/query_executor.hpp"

DltEngine::DltEngine(std::string db_name) : db_name(db_name) { }

ExecutionResult DltEngine::run(const std::string& sql) {
    auto start = std::chrono::high_resolution_clock::now();

    sql::SqlTokenizer tokenizer;
    std::vector<sql::SqlToken> tokens = tokenizer.tokenize(sql);

    sql::SqlParser parser(tokens);
    std::unique_ptr<sql::AstNode> node = parser.parse();

    exe::SemanticAnalyzer analyzer(db_name);
    analyzer.analyze(node.get());

    exe::QueryExecutor executor(db_name);
    auto result = executor.execute(*node);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return ExecutionResult(std::move(result), duration_ms);
}