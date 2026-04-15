//
// Created by poproshaikin on 12.11.25.
//

#ifndef DELTABASE_UUID_HPP
#define DELTABASE_UUID_HPP
#include <cstring>
#include <stdexcept>
#include <string>
#include <uuid.h>

namespace types
{
    struct UUID
    {
    private:
        uuid_t value_{};

        static void
        generate(uuid_t& out)
        {
            uuid_generate_time_v6(out);
        }

    public:
        UUID() = default;

        UUID(const uuid_t& other)
        {
            std::memcpy(&value_, other, sizeof(uuid_t));
        }

        UUID(const UUID& other)
        {
            std::memcpy(&value_, &other.value_, sizeof(uuid_t));
        }

        UUID&
        operator=(const UUID& other)
        {
            if (this != &other)
            {
                std::memcpy(&value_, &other.value_, sizeof(uuid_t));
            }

            return *this;
        }

        explicit UUID(const std::string& str)
        {
            if (uuid_parse(str.c_str(), value_) != 0)
                throw std::invalid_argument("Invalid UUID string: " + str);
        }

        static UUID
        make()
        {
            UUID uuid;
            generate(uuid.value_);
            return uuid;
        }

        static UUID
        null()
        {
            UUID uuid;
            uuid_clear(*uuid.raw());
            return uuid;
        }

        std::string
        to_string() const
        {
            char buffer[37];
            uuid_unparse_lower(value_, buffer);
            return std::string(buffer);
        }

        bool
        operator==(const UUID& other) const noexcept
        {
            return uuid_compare(value_, other.value_) == 0;
        }

        bool
        operator!=(const UUID& other) const noexcept
        {
            return !(*this == other);
        }

        uuid_t*
        raw() noexcept
        {
            return &value_;
        }

        const uuid_t*
        raw() const noexcept
        {
            return &value_;
        }
    };
} // namespace types

namespace std
{
    template <> struct hash<types::UUID>
    {
        size_t
        operator()(const types::UUID& uuid) const noexcept
        {
            auto sv = std::string_view(reinterpret_cast<const char*>(uuid.raw()), 16);

            return std::hash<std::string_view>{}(sv);
        }
    };
} // namespace std

#endif // DELTABASE_UUID_HPP