#ifndef CLI_CLI_HPP
#define CLI_CLI_HPP

#include "../../engine.hpp"
#include "../../sql/include/parser.hpp"
#include <functional>

extern "C" {
#include "../../core/include/data.h"
}

namespace cli {
    void
    print_ast_node(const std::unique_ptr<sql::AstNode>& node, int indent = 0);
    std::string
    token_to_string(const DataToken* token);
    void
    print_data_table(const DataTable& table);

    class SqlCli {
      public:
        SqlCli();
        void
        run_query_console();
        void
        add_cmd_handler(std::string cmd, std::function<void(std::string arg)> func);

      private:
        std::unique_ptr<DltEngine> engine;
        std::unordered_map<std::string, std::function<void(std::string arg)>> handlers;
        void
        proccess_input(std::string cmd);
    };
} // namespace cli

#endif
