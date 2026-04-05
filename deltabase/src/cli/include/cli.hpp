//
// Created by poproshaikin on 28.12.25.
//

#ifndef DELTABASE_CLI_HPP
#define DELTABASE_CLI_HPP
#include "cli_command_parser.hpp"
#include "cli_command.hpp"
#include "cli_context.hpp"
#include "engine.hpp"
#include "input_output.hpp"
#include "meta_executor.hpp"
#include "result_formatter.hpp"

namespace cli
{
    class Cli
    {
        CliContext ctx_;
        engine::Engine engine_;

        InputOutput io_;
        CliCommandParser parser_;
        MetaExecutor meta_exq_;
        ResultFormatter formatter_;

        void
        print_timer(std::chrono::time_point<std::chrono::steady_clock> start) const;

    public:
        Cli(const CliContext& ctx);

        void
        execute_meta(const CliCommand& command) noexcept(true);

        void
        execute_query(const CliCommand& command) noexcept(true);

        void
        run();
    };
}

#endif //DELTABASE_CLI_HPP