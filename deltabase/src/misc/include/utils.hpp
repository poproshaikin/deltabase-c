#ifndef MISC_UTILS_HPP
#define MISC_UTILS_HPP

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

std::vector<std::string>
split(const std::string& s, char delimiter, int count = 0);

template <typename Key, typename Value>
std::vector<Value>
get_values(std::unordered_map<Key, Value> map) {
    std::vector<Value> result;
    result.reserve(map.size());

    for (const auto& pair : map) {
        result.push_back(pair.second);
    }

    return result;
}

template <typename T>
inline T*
vector_to_c(const std::vector<T>& v) {
    T* arr = new T[v.size()];
    std::copy(v.begin(), v.end(), arr);
    return arr;
}

inline char**
string_vector_to_ptrs(const std::vector<std::string>& v) {
    char** result = new (std::nothrow) char*[v.size()];
    if (!result)
        return nullptr;

    for (size_t i = 0; i < v.size(); i++) {
        size_t len = v[i].size();
        result[i] = new (std::nothrow) char[len + 1];
        if (!result[i]) {
            for (size_t j = 0; j < i; j++) {
                delete[] result[j];
            }
            delete[] result;
            return nullptr;
        }

        std::memcpy(result[i], v[i].c_str(), len + 1);
    }

    return result;
}

char*
make_c_string(const std::string& str);

template <typename T>
inline T*
make_c_arr(const std::vector<T>& vec) {
    T* arr = new T[vec.size()];
    std::copy(vec.begin(), vec.end(), arr);
    return arr;
}

template <typename T>
inline T**
make_c_ptr_arr(const std::vector<T>& vec) {
    T** arr = new T*[vec.size()];

    for (size_t i = 0; i < vec.size(); ++i) {
        arr[i] = new T(vec[i]);
    }

    return arr;
}

void
print_ram_usage();

#endif