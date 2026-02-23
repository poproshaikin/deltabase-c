//
// Created by poproshaikin on 02.01.26.
//

#include "cli.hpp"

namespace cli
{
    using namespace types;

    Cli::Cli(const CliContext& ctx) : ctx_(ctx), io_(ctx_), meta_exq_(ctx_, engine_)
    {
        if (!ctx_.attached_db.empty())
            engine_.attach_db(ctx_.attached_db);
    }

    void
    Cli::execute_query(const CliCommand& command)
    {
        try
        {
            auto [query] = std::get<SqlCommand>(command);
            auto result = engine_.execute_query(query);

            auto formatted = formatter_.format(*result);
            io_.write(formatted);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Cli::execute_query failed: " + std::string(e.what()));
        }
    }

    void
    Cli::run()
    {
        while (ctx_.running)
        {
            std::string cmd = io_.get_command();
            CliCommand command = parser_.parse(cmd);

            auto type = std::visit(
                [](auto&& cmd)
                {
                    return cmd.type;
                },
                command
            );
            if (is_meta_command(type))
            {
                meta_exq_.execute(command);
                continue;
            }

            execute_query(command);
        }
    }
}