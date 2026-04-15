#ifndef MISC_UTILS_HPP
#define MISC_UTILS_HPP

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <uuid/uuid.h>
#include <stdexcept>

namespace misc
{
    std::vector<std::string>
    split(const std::string& s, char delimiter, int count = 0);

    template <typename Key, typename Value>
    std::vector<Value>
    get_values(std::unordered_map<Key, Value> map)
    {
        std::vector<Value> result;
        result.reserve(map.size());

        for (const auto& pair : map)
        {
            result.push_back(pair.second);
        }

        return result;
    }

    template <typename T>
    T
    *
    vector_to_c(const std::vector<T>& v)
    {
        T* arr = new T[v.size()];
        std::copy(v.begin(), v.end(), arr);
        return arr;
    }

    inline char
    **
    string_vector_to_ptrs(const std::vector<std::string>& v)
    {
        char** result = new(std::nothrow) char*[v.size()];
        if (!result)
            return nullptr;

        for (size_t i = 0; i < v.size(); i++)
        {
            size_t len = v[i].size();
            result[i] = new(std::nothrow) char[len + 1];
            if (!result[i])
            {
                for (size_t j = 0; j < i; j++)
                {
                    delete[] result[j];
                }
                delete[] result;
                return nullptr;
            }

            std::memcpy(result[i], v[i].c_str(), len + 1);
        }

        return result;
    }

    char
    *
    make_c_string(const std::string& str);

    inline uint64_t
    to_big_endian_u64(uint64_t value)
    {
        return ((value & 0x00000000000000FFULL) << 56)
            | ((value & 0x000000000000FF00ULL) << 40)
            | ((value & 0x0000000000FF0000ULL) << 24)
            | ((value & 0x00000000FF000000ULL) << 8)
            | ((value & 0x000000FF00000000ULL) >> 8)
            | ((value & 0x0000FF0000000000ULL) >> 24)
            | ((value & 0x00FF000000000000ULL) >> 40)
            | ((value & 0xFF00000000000000ULL) >> 56);
    }

    inline uint64_t
    from_big_endian_u64(uint64_t value)
    {
        return to_big_endian_u64(value);
    }

    void
    print_ram_usage();
}

template <typename E>
bool has_flag(E value, E flag)
{
    using T = std::underlying_type_t<E>;
    return (static_cast<T>(value) & static_cast<T>(flag)) != 0;
}

#endif