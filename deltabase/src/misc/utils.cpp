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

template<typename Key, typename Value>
std::vector<Value> get_values(std::unordered_map<Key, Value> map) {
    std::vector<Value> result;
    result.reserve(map.size()); 

    for (const auto& pair : map) {
        result.push_back(pair.second);
    }

    return result;
}
