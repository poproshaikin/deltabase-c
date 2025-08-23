#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <chrono>

#include "../src/executor/include/semantic_analyzer.hpp"
#include "../src/executor/include/query_executor.hpp"
#include "../src/cli/include/cli.hpp"

extern "C" {
    #include "../src/core/include/core.h"
    #include "../src/core/include/meta.h"
    #include "../src/core/include/data.h"
}

using namespace sql;

namespace tests {
    // Simple AST printing - just for tests, not complete
    void print_ast_node(const std::unique_ptr<sql::AstNode>& node, int indent = 0) {
        if (!node) {
            std::cout << std::string(indent, ' ') << "(null)\n";
            return;
        }

        std::string indent_str(indent, ' ');
        std::cout << indent_str << "Node type: " << static_cast<int>(node->type) << "\n";
        
        // Simplified printing - just showing it compiles and works
        std::visit([&](auto&& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, sql::SqlToken>) {
                std::cout << indent_str << "  SqlToken: " << value.to_string() << "\n";
            } else {
                std::cout << indent_str << "  Other AST node type\n";
            }
        }, node->value);
    }

    std::string token_to_string(const DataToken* token) {
        if (!token || token->type == DT_NULL) {
            return "NULL";
        }

        switch (token->type) {
            case DT_INTEGER: {
                int32_t value;
                memcpy(&value, token->bytes, sizeof(value));
                return std::to_string(value);
            }
            case DT_REAL: {
                double value;
                memcpy(&value, token->bytes, sizeof(value));
                return std::to_string(value);
            }
            case DT_BOOL: {
                bool value;
                memcpy(&value, token->bytes, sizeof(value));
                return value ? "true" : "false";
            }
            case DT_CHAR: {
                return std::string(1, *token->bytes);
            }
            case DT_STRING: {
                return std::string(token->bytes, token->size);
            }
            default:
                return "<unknown>";
        }
    }

    void print_data_table(const DataTable* table) {
        if (!table || !table->scheme) {
            std::cerr << "Invalid DataTable\n";
            return;
        }

        const MetaTable* schema = table->scheme;

        for (size_t i = 0; i < schema->columns_count; ++i) {
            std::cout << std::left << std::setw(15) << schema->columns[i]->name;
        }
        std::cout << "\n";

        for (size_t i = 0; i < schema->columns_count; ++i) {
            std::cout << std::setw(15) << std::setfill('-') << "";
        }
        std::cout << std::setfill(' ') << "\n";

        for (size_t r = 0; r < table->rows_count; ++r) {
            const DataRow* row = table->rows[r];
            for (size_t c = 0; c < schema->columns_count; ++c) {
                const DataToken* token = row->tokens[c];
                std::cout << std::left << std::setw(15) << token_to_string(token);
            }
            std::cout << "\n";
        }
    }

    void process_input(const std::string& input) {
        std::string db_name = "testdb";

        auto start = std::chrono::high_resolution_clock::now();

        sql::SqlTokenizer tokenizer;
        std::vector<sql::SqlToken> tokens = tokenizer.tokenize(input);

        sql::SqlParser parser(tokens);
        std::unique_ptr<sql::AstNode> node = parser.parse();

        print_ast_node(node);

        exe::SemanticAnalyzer analyzer(db_name);
        analyzer.analyze(node.get());

        exe::QueryExecutor executor(db_name);
        const auto result = executor.execute(*node);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - start);
        std::cout << "Time elapsed: " << duration.count() << " ms" << std::endl;

        if (std::holds_alternative<std::unique_ptr<DataTable>>(result))
            print_data_table(std::get<std::unique_ptr<DataTable>>(result).get());
        else 
            std::cout << "Rows affected: " << std::get<int>(result) << std::endl;
    }

    void run_test_console() {
        std::string input;
        std::cout << "Welcome to deltabase test console! Type 'exit' to quit.\n";

        while (true) {
            std::cout << "test> ";
            if (!std::getline(std::cin, input)) {
                break;
            }
            if (input == "exit") {
                break;
            }
            try {
                process_input(input);
            } catch(const std::runtime_error& e) {
                std::cout << "Error: " << e.what() << std::endl;
            }
        }

        std::cout << "Exiting deltabase test console.\n";
    }
}

int main() {
    std::cout << "Starting deltabase tests...\n";
    
    // Simple test
    try {
        tests::process_input("SELECT id, name FROM users WHERE id > 10");
    } catch(const std::runtime_error& e) {
        std::cout << "Test error: " << e.what() << std::endl;
    }
    
    // Run interactive console
    tests::run_test_console();
    
    return 0;
}
