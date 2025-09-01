#ifndef CLI_CLI_HPP
#define CLI_CLI_HPP

#include "../../engine/include/engine.hpp"
#include "../../sql/include/parser.hpp"
#include <functional>

extern "C" {
#include "../../core/include/data.h"
}

namespace cli {
    void
    print_ast_node(const std::unique_ptr<sql::AstNode>& node, int indent = 0);

    auto
    token_to_string(const DataToken* token) -> std::string;
    
    void
    print_data_table(const DataTable& table);

    class SqlCli {
        std::unique_ptr<engine::DltEngine> engine_;
        std::unordered_map<std::string, std::function<void(std::string arg)>> handlers_;

        void
        proccess_input(std::string cmd);

      public:
        SqlCli();

        void
        run_query_console();

        void
        run_cmd(const std::string& cmd);

        void
        add_cmd_handler(std::string cmd, std::function<void(std::string arg)> func);
    };
} // namespace cli

#endif
