//
// Created by poproshaikin on 02.01.26.
//

#include "meta_executor.hpp"
#include "../engine/include/engine.hpp"
#include "cli_context.hpp"

#include <stdexcept>

namespace cli
{
    MetaExecutor::MetaExecutor(CliContext& ctx, engine::Engine& engine) : ctx_(ctx), engine_(engine)
    {
    }

    void
    MetaExecutor::execute(const CliCommand& command) const
    {
        switch (std::visit([](auto& cmd) { return cmd.type; }, command))
        {
        case CommandType::EXIT:
        {
            engine_.detach_db();
            ctx_.running = false;
            break;
        }
        case CommandType::CONNECT:
        {
            const auto& db_name = std::get<ConnectCommand>(command).db_name;
            ctx_.attached_db = db_name;
            engine_.attach_db(db_name);
            break;
        }
        default:
            throw std::runtime_error("MetaExecutor::execute: unknown type of meta command");
        }
    }
} // namespace cli