//
// Created by poproshaikin on 02.01.26.
//

#include "input_output.hpp"

#include <ostream>
#include <istream>
extern "C"
{
#include "../../third_party/linenoise-ng/linenoise.h"
}

namespace cli
{
    InputOutput::InputOutput(CliContext& ctx) : ctx_(ctx)
    {
    }

    std::string
    InputOutput::get_command() const
    {
        std::string prompt = (!ctx_.attached_db.empty() ? ctx_.attached_db : "db") + "> ";

        char* line = linenoise(prompt.c_str());
        if (!line)
            return {}; // EOF (Ctrl+D)

        std::string command(line);
        free(line);

        if (!command.empty()) {
            linenoiseHistoryAdd(command.c_str());
            linenoiseHistorySetMaxLen(100);
        }

        return command;
    }

    void
    InputOutput::write(const std::string& str) const
    {
        ctx_.out << str << std::flush;
    }
}