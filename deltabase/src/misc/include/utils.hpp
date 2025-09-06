#ifndef MISC_UTILS_HPP
#define MISC_UTILS_HPP

#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include <uuid/uuid.h>
}

std::vector<std::string>
split(const std::string& s, char delimiter);

template <typename Key, typename Value>
std::vector<Value>
get_values(std::unordered_map<Key, Value> map);

template <typename Type>
inline Type*
vector_to_c(const std::vector<Type>& v) {
    Type *arr = new Type[v.size()];
    std::copy(v.begin(), v.end(), arr);
    return arr;
}

inline char**
string_vector_to_ptrs(const std::vector<std::string>& v) {
    char **result = new (std::nothrow) char*[v.size()];
    if (!result) return nullptr;

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

#endif

char*
make_c_string(const std::string& str);

template <typename T>
T*
make_c_arr(const std::vector<T>& vec);

template <typename T>
T**
make_c_ptr_arr(const std::vector<T>& vec);

std::string
make_uuid_str();

std::string
make_uuid_str(const uuid_t uuid);

void
parse_uuid_str(const std::string& str, uuid_t uuid);