#ifndef MISC_UTILS_HPP
#define MISC_UTILS_HPP

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace misc
{
    std::vector<std::string>
    split(const std::string& s, char delimiter, int count = 0);

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

    inline int32_t
    to_big_endian_i32(int32_t value)
    {
        const auto u = static_cast<uint32_t>(value);
        return static_cast<int32_t>(
            ((u & 0x000000FFu) << 24)
          | ((u & 0x0000FF00u) << 8)
          | ((u & 0x00FF0000u) >> 8)
          | ((u & 0xFF000000u) >> 24)
        );
    }

    inline int32_t
    from_big_endian_i32(int32_t value)
    {
        return to_big_endian_i32(value);
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