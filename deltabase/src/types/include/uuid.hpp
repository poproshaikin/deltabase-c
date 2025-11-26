//
// Created by poproshaikin on 12.11.25.
//

#ifndef DELTABASE_UUID_HPP
#define DELTABASE_UUID_HPP
#include <stdexcept>
#include <string>
#include <uuid.h>

namespace types
{
    struct Uuid
    {
        Uuid()
        {
            generate(value_);
        }

        explicit Uuid(const std::string& str)
        {
            if (uuid_parse(str.c_str(), value_) != 0)
                throw std::invalid_argument("Invalid UUID string: " + str);
        }

        std::string to_string() const
        {
            char buffer[37];
            uuid_unparse_lower(value_, buffer);
            return std::string(buffer);
        }

        bool operator==(const Uuid& other) const noexcept
        {
            return uuid_compare(value_, other.value_) == 0;
        }

        bool operator!=(const Uuid& other) const noexcept
        {
            return !(*this == other);
        }

        const uuid_t& raw() const noexcept { return value_; }

    private:
        uuid_t value_{};

        static void generate(uuid_t& out)
        {
            uuid_generate_time_v6(out);
        }
    };
}

namespace std
{
    template<>
    struct hash<types::Uuid>
    {
        size_t operator()(const types::Uuid& uuid) const noexcept
        {
            auto sv = std::string_view(
                reinterpret_cast<const char*>(uuid.raw()),
                16
            );

            return std::hash<std::string_view>{}(sv);
        }
    };
}

#endif //DELTABASE_UUID_HPP