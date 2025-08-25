#include "include/utils.hpp"

#include <string>
#include <sstream>
#include <vector>

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }

    return result;
}

char *make_c_string(const std::string& str) {
    char *ptr = new char[str.size() + 1];
    std::memcpy(ptr, str.c_str(), str.size() + 1);
    
    return ptr;
}