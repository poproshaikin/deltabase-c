//
// Created by poproshaikin on 02.01.26.
//

#ifndef DELTABASE_META_EXECUTOR_HPP
#define DELTABASE_META_EXECUTOR_HPP

#include "cli_context.hpp"
#include "cli_command.hpp"
#include "../../engine/include/engine.hpp"


namespace cli
{
    class MetaExecutor
    {
        CliContext& ctx_;
        engine::Engine& engine_;

    public:
        explicit
        MetaExecutor(CliContext& ctx, engine::Engine& engine);

        void
        execute(const CliCommand& command) const;
    };
}

#endif //DELTABASE_META_EXECUTOR_HPP