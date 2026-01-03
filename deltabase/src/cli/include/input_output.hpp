//
// Created by poproshaikin on 02.01.26.
//

#ifndef DELTABASE_INPUT_READER_HPP
#define DELTABASE_INPUT_READER_HPP
#include "cli_context.hpp"

#include <string>

namespace cli
{
    class InputOutput
    {
        CliContext& ctx_;

    public:
        explicit
        InputOutput(CliContext& ctx);

        std::string
        get_command() const;

        void
        write(const std::string& str) const;
    };
}

#endif //DELTABASE_INPUT_READER_HPP