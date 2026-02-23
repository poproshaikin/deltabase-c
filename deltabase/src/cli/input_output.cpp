//
// Created by poproshaikin on 02.01.26.
//

#include "input_output.hpp"

#include <ostream>
#include <istream>

namespace cli
{
    InputOutput::InputOutput(CliContext& ctx) : ctx_(ctx)
    {
    }

    std::string
    InputOutput::get_command() const
    {
        std::string line;
        ctx_.out << (!ctx_.attached_db.empty() ? ctx_.attached_db : std::string("db")) <<
            std::string(">") << std::flush;

        if (!std::getline(ctx_.in, line))
            return {}; // EOF

        return line;
    }

    void
    InputOutput::write(const std::string& str) const
    {
        ctx_.out << str << std::flush;
    }
}