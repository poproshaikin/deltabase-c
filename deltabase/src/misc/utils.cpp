#include "include/utils.hpp"

#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace misc
{
    std::vector<std::string>
    split(const std::string& s, char delimiter, int count)
    {
        std::vector<std::string> result;
        std::stringstream ss(s);
        std::string item;
        int counter = 0;

        while (std::getline(ss, item, delimiter))
        {
            if (count != 0 && counter == count)
            {
                std::string rest;
                std::getline(ss, rest, '\0');
                if (!rest.empty())
                {
                    item += delimiter + rest;
                }
                result.push_back(item);
                return result;
            }
            result.push_back(item);
            ++counter;
        }

        return result;
    }

    auto
    make_c_string(const std::string& str) -> char*
    {
        char* ptr = new char[str.size() + 1];
        std::memcpy(ptr, str.c_str(), str.size() + 1);
        return ptr;
    }

    void
    print_ram_usage()
    {
        std::ifstream statm("/proc/self/status");
        std::string line;
        while (std::getline(statm, line))
        {
            if (line.starts_with("VmRSS:"))
            {
                std::cout << line << std::endl;
            }
        }
    }
}
