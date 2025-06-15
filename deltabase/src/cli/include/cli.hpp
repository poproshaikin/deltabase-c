#pragma once

#include <sstream>
extern "C" {
    #include "../../core/include/data_table.h"
}

namespace cli {
    class CoreCli {
        public:
            int execute(std::string input, std::ostringstream& output);
    };
}
