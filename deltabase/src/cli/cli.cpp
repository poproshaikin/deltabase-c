#include "include/cli.hpp"
#include "../misc/include/utils.hpp"
#include <vector>

extern "C" {
    #include "../core/include/core.h"
}

using namespace cli;

int CoreCli::execute(std::string input, std::ostringstream& output) {
    std::vector<std::string> tokenized = split(input);   

    int rc = 0;

    if (tokenized[0] == "scan") {
        if (tokenized.size() != 3) {
            output << "Invalid arguments count";
        }

        DataTable table;
        if ((rc = full_scan(tokenized[1].data(), tokenized[2].data(), &table)) != 0) {
            output << "Something went wrong" << std::endl << "Error code: " << rc << std::endl;
            return rc;
        }

    }

    return 0;
}
