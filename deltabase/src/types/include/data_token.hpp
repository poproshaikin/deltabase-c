//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_DATA_TOKEN_HPP
#define DELTABASE_DATA_TOKEN_HPP

#include "sql_token.hpp"
#include "typedefs.hpp"
#include "data_type.hpp"

#include <cstring>
#include <stdexcept>

namespace types
{
    template <typename T>
    concept AllowedDataTypes_c =
        std::same_as<T, int> ||
        std::same_as<T, double> ||
        std::same_as<T, std::string> ||
        std::same_as<T, bool> ||
        std::same_as<T, char>;

    struct DataToken
    {
        Bytes bytes;
        DataType type;

        DataToken() = default;

        explicit
        DataToken(const SqlToken& sql_token);

        explicit
        DataToken(const Bytes& bytes, DataType type);

        // Returns the common type to use when comparing a and b.
        // Returns DataType::UNDEFINED if the types are not comparable.
        static DataType common_type(DataType a, DataType b);

        // Type-aware comparison. Widens operands to common_type if needed.
        // Precondition: both tokens are non-NULL and common_type(a.type, b.type) != UNDEFINED.
        // For NULL handling see Evaluator::evaluate.
        static int compare(const DataToken& a, const DataToken& b);

        // For display only (result_formatter). Do not use in comparisons.
        template <AllowedDataTypes_c T>
        T as() const
        {
            if constexpr (std::is_same_v<T, int>)
            {
                if (bytes.size() != sizeof(int))
                    throw std::runtime_error("DataToken::as<int>: size mismatch");
                int v;
                std::memcpy(&v, bytes.data(), sizeof(int));
                return v;
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                if (bytes.size() != sizeof(double))
                    throw std::runtime_error("DataToken::as<double>: size mismatch");
                double v;
                std::memcpy(&v, bytes.data(), sizeof(double));
                return v;
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                if (bytes.empty())
                    throw std::runtime_error("DataToken::as<bool>: empty buffer");
                return bytes[0] != 0;
            }
            else if constexpr (std::is_same_v<T, char>)
            {
                if (bytes.empty())
                    throw std::runtime_error("DataToken::as<char>: empty buffer");
                return static_cast<char>(bytes[0]);
            }
            else
            {
                return std::string(bytes.begin(), bytes.end());
            }
        }
    };

    bool operator==(const DataToken& lhs, const DataToken& rhs);
    bool operator!=(const DataToken& lhs, const DataToken& rhs);
    bool operator<(const DataToken& lhs, const DataToken& rhs);
    bool operator<=(const DataToken& lhs, const DataToken& rhs);
    bool operator>(const DataToken& lhs, const DataToken& rhs);
    bool operator>=(const DataToken& lhs, const DataToken& rhs);
}

#endif //DELTABASE_DATA_TOKEN_HPP
