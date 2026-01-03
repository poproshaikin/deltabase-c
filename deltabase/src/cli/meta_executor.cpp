//
// Created by poproshaikin on 02.01.26.
//

#include "meta_executor.hpp"
#include "cli_context.hpp"

#include <stdexcept>

namespace cli
{
    MetaExecutor::MetaExecutor(CliContext& ctx) : ctx_(ctx)
    {
    }

    void
    MetaExecutor::execute(const CliCommand& command) const
    {
        switch (std::visit([](auto& cmd) { return cmd.type; }, command))
        {
        case CommandType::EXIT:
            ctx_.running = false;
            break;
        case CommandType::CONNECT:
            ctx_.attached_db = std::get<ConnectCommand>(command).db_name;
            break;
        default:
            throw std::runtime_error("MetaExecutor::execute: unknown type of meta command");
        }
    }
}