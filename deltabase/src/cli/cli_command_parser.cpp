//
// Created by poproshaikin on 02.01.26.
//

#include "cli_command_parser.hpp"

namespace cli
{
    CliCommand
    CliCommandParser::parse(const std::string& cmd) const
    {
        if (cmd.starts_with(".q"))
            return ExitCommand();

        if (cmd.starts_with(".c "))
            return ConnectCommand(cmd.substr(3)); // len of '.c '

        return SqlCommand(cmd);
    }
}