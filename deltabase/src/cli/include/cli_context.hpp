//
// Created by poproshaikin on 02.01.26.
//

#ifndef DELTABASE_CLI_CONTEXT_HPP
#define DELTABASE_CLI_CONTEXT_HPP
#include <string>

namespace cli
{
    struct CliContext
    {
        bool running = true;
        std::string attached_db;
        std::istream& in;
        std::ostream& out;
    };
}

#endif //DELTABASE_CLI_CONTEXT_HPP