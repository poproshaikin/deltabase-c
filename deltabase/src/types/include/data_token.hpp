//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_DATA_TOKEN_HPP
#define DELTABASE_DATA_TOKEN_HPP

#include "sql_token.hpp"
#include "typedefs.hpp"
#include "data_type.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>

namespace types
{
    template <typename T>
    concept AllowedDataTypes_c = std::same_as<T, int> ||
                                 std::same_as<T, double> ||
                                 std::same_as<T, std::string> ||
                                 std::same_as<T, bool> ||
                                 std::same_as<T, char>;

    struct DataToken
    {
        Bytes bytes;
        DataType type;

        explicit
        DataToken(const SqlToken& sql_token);
        explicit
        DataToken(const Bytes& bytes, DataType type);

        uint64_t
        estimate_size() const;

        template <AllowedDataTypes_c T>
        T
        as() const
        {
            static_assert(std::is_same_v<T, int>
                          || std::is_same_v<T, double>
                          || std::is_same_v<T, bool>
                          || std::is_same_v<T, char>
                          || std::is_same_v<T, std::string>,
                          "DataToken::as<T>() not implemented for this type");

            if constexpr (std::is_same_v<T, int>)
            {
                assert(bytes.size() == sizeof(int));
                int v;
                std::memcpy(&v, bytes.data(), sizeof(int));
                return v;
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                assert(bytes.size() == sizeof(double));
                double v;
                std::memcpy(&v, bytes.data(), sizeof(double));
                return v;
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                assert(!bytes.empty());
                return bytes[0] != 0;
            }
            else if constexpr (std::is_same_v<T, char>)
            {
                assert(!bytes.empty());
                return static_cast<char>(bytes[0]);
            }
            else
            {
                return std::string(bytes.begin(), bytes.end());
            }
        }

        Bytes
        serialize() const;
    };

}

#endif //DELTABASE_DATA_TOKEN_HPP