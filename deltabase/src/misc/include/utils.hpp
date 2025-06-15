#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "../../sql/include/lexer.hpp"

std::vector<std::string> split(const std::string& s, char delimiter);

template<typename Key, typename Value>
std::vector<Value> get_values(std::unordered_map<Key, Value> map);
