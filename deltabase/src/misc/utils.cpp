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

char *make_c_string(const std::string& str) {
    char *ptr = new char[str.size() + 1];
    std::memcpy(ptr, str.c_str(), str.size() + 1);
    
    return ptr;
}

template <typename T>
T* make_c_arr(const std::vector<T>& vec) {
    T* arr = new T[vec.size()];
    std::copy(vec.begin(), vec.end(), arr);
    return arr;
}

template <typename T>
T** make_c_ptr_arr(const std::vector<T>& vec) {
    T** arr = new T*[vec.size()];

    for (size_t i = 0; i < vec.size(); ++i) {
        arr[i] = new T(vec[i]);
    }

    return arr; 
}