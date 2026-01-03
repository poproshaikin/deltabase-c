//
// Created by poproshaikin on 02.01.26.
//

#ifndef DELTABASE_META_EXECUTOR_HPP
#define DELTABASE_META_EXECUTOR_HPP

#include "cli_context.hpp"
#include "cli_command.hpp"

namespace cli
{
    class MetaExecutor
    {
        CliContext& ctx_;

    public:
        explicit
        MetaExecutor(CliContext& ctx);

        void
        execute(const CliCommand& command) const;
    };
}

#endif //DELTABASE_META_EXECUTOR_HPP