//
// Created by poproshaikin on 02.01.26.
//

#include "cli.hpp"

#include <chrono>

namespace cli
{
    using namespace types;

    Cli::Cli(const CliContext& ctx) : ctx_(ctx), io_(ctx_), meta_exq_(ctx_, engine_)
    {
        if (!ctx_.attached_db.empty())
            engine_.attach_db(ctx_.attached_db);
    }

    void
    Cli::print_timer(std::chrono::time_point<std::chrono::steady_clock> started_at) const
    {

        const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                    std::chrono::steady_clock::now() - started_at
        )
                                    .count();
        const auto elapsed_ms = elapsed_ns / 1000000;
        const auto ns_fraction = elapsed_ns % 1000000;

        std::string fraction = std::to_string(ns_fraction);
        if (fraction.size() < 6)
            fraction.insert(0, 6 - fraction.size(), '0');

        io_.write("Query took: " + std::to_string(elapsed_ms) + "." + fraction + " ms\n");
    }

    void
    Cli::execute_meta(const CliCommand& command) noexcept(true)
    {
        try
        {
            meta_exq_.execute(command);
        }
        catch (const std::exception& e)
        {
            std::cerr << "ERR: " << e.what() << std::endl;
        }
    }

    void
    Cli::execute_query(const CliCommand& command) noexcept(true)
    {
        try
        {
            const auto started_at = std::chrono::steady_clock::now();

            auto [query] = std::get<SqlCommand>(command);
            auto result = engine_.execute_query(query);

            auto formatted = formatter_.format(*result);
            io_.write(formatted);

            print_timer(started_at);
        }
        catch (const std::exception& e)
        {
            std::cerr << "ERR: " << e.what() << std::endl;
        }
    }

    void
    Cli::run()
    {
        while (ctx_.running)
        {
            std::string cmd = io_.get_command();

            CliCommand command = parser_.parse(cmd);

            auto type = std::visit([](auto&& cmd) { return cmd.type; }, command);
            if (is_meta_command(type))
            {
                execute_meta(command);
            }
            else
            {
                execute_query(command);
            }
        }
    }
} // namespace cli