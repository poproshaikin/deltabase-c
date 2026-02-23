//
// Created by poproshaikin on 02.01.26.
//

#ifndef DELTABASE_META_COMMAND_PARSER_HPP
#define DELTABASE_META_COMMAND_PARSER_HPP
#include "cli_command.hpp"

namespace cli
{
    class CliCommandParser
    {
    public:
        CliCommand
        parse(const std::string& cmd) const;
    };
}

#endif //DELTABASE_META_COMMAND_PARSER_HPP