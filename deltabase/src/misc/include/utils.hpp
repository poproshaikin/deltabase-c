#ifndef MISC_UTILS_HPP
#define MISC_UTILS_HPP

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <uuid/uuid.h>

auto
split(const std::string& s, char delimiter, int count = 0) -> std::vector<std::string>;

template <typename Key, typename Value>
auto
get_values(std::unordered_map<Key, Value> map) -> std::vector<Value> {
    std::vector<Value> result;
    result.reserve(map.size());

    for (const auto& pair : map) {
        result.push_back(pair.second);
    }

    return result;
}

template <typename T>
inline auto
vector_to_c(const std::vector<T>& v) -> T* {
    T* arr = new T[v.size()];
    std::copy(v.begin(), v.end(), arr);
    return arr;
}

inline auto
string_vector_to_ptrs(const std::vector<std::string>& v) -> char** {
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

auto
make_c_string(const std::string& str) -> char*;

template <typename T>
inline auto
make_c_arr(const std::vector<T>& vec) -> T* {
    T* arr = new T[vec.size()];
    std::copy(vec.begin(), vec.end(), arr);
    return arr;
}

template <typename T>
inline auto
make_c_ptr_arr(const std::vector<T>& vec) -> T** {
    T** arr = new T*[vec.size()];

    for (size_t i = 0; i < vec.size(); ++i) {
        arr[i] = new T(vec[i]);
    }

    return arr;
}

void
print_ram_usage();

std::string
make_uuid_str();

std::string
make_uuid_str(const uuid_t uuid);

void
parse_uuid_str(const std::string& str, uuid_t uuid);

#endif