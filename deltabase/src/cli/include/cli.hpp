#pragma once

#include <memory>
#include <string>

#include "../../executor/include/query_executor.hpp"

extern "C" {
    #include "../../core/include/data_table.h"
}

namespace cli {
    void print_ast_node(const std::unique_ptr<sql::AstNode>& node, int indent = 0);
    std::string token_to_string(const DataToken* token);
    void print_data_table(const DataTable& table);
    

    class SqlCli {
        public:
            void run_query_console();
        private:
            void process_input(const std::string& input);
    };
}
