#pragma once

#include <string>
#include <vector>

namespace cli {
    struct Argument {
        std::string name;
        void *value;
    };

    struct Command {
        std::string name;   
        std::vector<Argument> arguments;
    };
}
