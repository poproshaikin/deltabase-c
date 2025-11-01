#include "include/utils.hpp"

#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

auto
split(const std::string& s, char delimiter, int count) -> std::vector<std::string> {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    int counter = 0;

    while (std::getline(ss, item, delimiter)) {
        if (count != 0 && counter == count) {
            std::string rest;
            std::getline(ss, rest, '\0');
            if (!rest.empty()) {
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
make_c_string(const std::string& str) -> char* {
    char* ptr = new char[str.size() + 1];
    std::memcpy(ptr, str.c_str(), str.size() + 1);
    return ptr;
}

void
print_ram_usage() {
    std::ifstream statm("/proc/self/status");
    std::string line;
    while (std::getline(statm, line)) {
        if (line.starts_with("VmRSS:")) { 
            std::cout << line << std::endl;
        }
    }
}

std::string
make_uuid_str() {
    char str[37];
    uuid_t uuid;

    uuid_generate_time(uuid);
    uuid_unparse(uuid, str);

    return std::string(str, 37);
}

std::string
make_uuid_str(const uuid_t uuid) {
    char str[37];
    uuid_unparse(uuid, str);
    return std::string(str, 37);
}

void
parse_uuid_str(const std::string& str, uuid_t uuid) {
    uuid_parse(str.c_str(), uuid);
}