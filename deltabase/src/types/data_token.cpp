//
// Created by poproshaikin on 13.11.25.
//

#include "include/data_token.hpp"

#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace types
{
    namespace
    {
        // Maps (left_type, right_type) → common type for comparison.
        // Add a new pair here to support cross-type comparisons.
        const std::unordered_map<DataType, std::unordered_map<DataType, DataType>>&
        coerce_table()
        {
            static const std::unordered_map<DataType, std::unordered_map<DataType, DataType>> table =
            {
                { DataType::INTEGER, {
                    { DataType::INTEGER, DataType::INTEGER },
                    { DataType::REAL,    DataType::REAL    },  // int op real → widen to real
                }},
                { DataType::REAL, {
                    { DataType::REAL,    DataType::REAL    },
                    { DataType::INTEGER, DataType::REAL    },  // real op int → widen to real
                }},
                { DataType::TEXT, {
                    { DataType::TEXT,    DataType::TEXT    },
                }},
                { DataType::CHAR, {
                    { DataType::CHAR,    DataType::CHAR    },
                }},
                { DataType::BOOL, {
                    { DataType::BOOL,    DataType::BOOL    },
                }},
            };
            return table;
        }

        // Widen token to target type. Only non-trivial case: INTEGER → REAL.
        DataToken widen(const DataToken& token, DataType target)
        {
            if (token.type == target)
                return token;

            if (token.type == DataType::INTEGER && target == DataType::REAL)
            {
                int iv;
                std::memcpy(&iv, token.bytes.data(), sizeof(int));
                double dv = static_cast<double>(iv);
                Bytes b(sizeof(double));
                std::memcpy(b.data(), &dv, sizeof(double));
                return DataToken(b, DataType::REAL);
            }

            return token; // no-op for same-type calls
        }

        // Signed integer stored in little-endian 2's complement.
        int compare_signed_le(const uint8_t* a, const uint8_t* b, size_t n)
        {
            const bool a_neg = (a[n - 1] & 0x80) != 0;
            const bool b_neg = (b[n - 1] & 0x80) != 0;
            if (a_neg != b_neg)
                return a_neg ? -1 : 1;
            for (int i = static_cast<int>(n) - 1; i >= 0; --i)
            {
                if (a[i] != b[i])
                    return (a[i] < b[i]) ? -1 : 1;
            }
            return 0;
        }

        // IEEE 754 floating-point stored in little-endian.
        int compare_ieee754_le(const uint8_t* a, const uint8_t* b, size_t n)
        {
            const bool a_neg = (a[n - 1] & 0x80) != 0;
            const bool b_neg = (b[n - 1] & 0x80) != 0;
            if (a_neg != b_neg)
                return a_neg ? -1 : 1;
            for (int i = static_cast<int>(n) - 1; i >= 0; --i)
            {
                if (a[i] != b[i])
                {
                    const int cmp = (a[i] < b[i]) ? -1 : 1;
                    return a_neg ? -cmp : cmp;
                }
            }
            return 0;
        }

        // Lexicographic byte comparison (TEXT).
        int compare_bytes_lex(const Bytes& a, const Bytes& b)
        {
            const size_t n = std::min(a.size(), b.size());
            for (size_t i = 0; i < n; ++i)
            {
                if (a[i] != b[i])
                    return (a[i] < b[i]) ? -1 : 1;
            }
            if (a.size() != b.size())
                return (a.size() < b.size()) ? -1 : 1;
            return 0;
        }

        int compare_same_type(const DataToken& a, const DataToken& b)
        {
            switch (a.type)
            {
            case DataType::INTEGER:
                return compare_signed_le(a.bytes.data(), b.bytes.data(), a.bytes.size());
            case DataType::REAL:
                return compare_ieee754_le(a.bytes.data(), b.bytes.data(), a.bytes.size());
            case DataType::TEXT:
                return compare_bytes_lex(a.bytes, b.bytes);
            case DataType::CHAR:
            case DataType::BOOL:
                return static_cast<int>(a.bytes[0]) - static_cast<int>(b.bytes[0]);
            default:
                return 0;
            }
        }
    } // namespace

    DataToken::DataToken(const SqlToken& sql_token)
    {
        if (!sql_token.is_literal())
            throw std::invalid_argument("Cannot convert non-literal SQL token to a data token");

        auto literal_type = sql_token.get_detail<SqlLiteral>();

        switch (literal_type)
        {
        case SqlLiteral::INTEGER:
        {
            int temp = std::stoi(sql_token.value);
            bytes.resize(sizeof(int));
            std::memcpy(bytes.data(), &temp, sizeof(int));
            type = DataType::INTEGER;
            break;
        }
        case SqlLiteral::REAL:
        {
            double temp = std::stod(sql_token.value);
            bytes.resize(sizeof(double));
            std::memcpy(bytes.data(), &temp, sizeof(double));
            type = DataType::REAL;
            break;
        }
        case SqlLiteral::STRING:
        {
            bytes.assign(sql_token.value.begin(), sql_token.value.end());
            type = DataType::TEXT;
            break;
        }
        case SqlLiteral::BOOL:
        {
            bytes.resize(1);
            bytes[0] = (sql_token.value == "true" || sql_token.value == "1") ? 1 : 0;
            type = DataType::BOOL;
            break;
        }
        case SqlLiteral::CHAR:
        {
            bytes.resize(1);
            bytes[0] = static_cast<uint8_t>(sql_token.value[0]);
            type = DataType::CHAR;
            break;
        }
        case SqlLiteral::_NULL:
        {
            bytes.clear();
            type = DataType::_NULL;
            break;
        }
        default:
            throw std::invalid_argument(
                "Cannot convert SQL token to a data token: invalid value type: " + sql_token.value
            );
        }
    }

    DataToken::DataToken(const Bytes& bytes, DataType type) : bytes(bytes), type(type)
    {
    }

    DataType DataToken::common_type(DataType a, DataType b)
    {
        const auto& table = coerce_table();
        auto outer = table.find(a);
        if (outer == table.end()) return DataType::UNDEFINED;
        auto inner = outer->second.find(b);
        if (inner == outer->second.end()) return DataType::UNDEFINED;
        return inner->second;
    }

    int DataToken::compare(const DataToken& a, const DataToken& b)
    {
        if (a.type == b.type)
            return compare_same_type(a, b);

        const DataType common = common_type(a.type, b.type);
        return compare_same_type(widen(a, common), widen(b, common));
    }

    bool operator==(const DataToken& lhs, const DataToken& rhs)
    {
        if (lhs.type == DataType::_NULL && rhs.type == DataType::_NULL) return true;
        if (DataToken::common_type(lhs.type, rhs.type) == DataType::UNDEFINED) return false;
        return DataToken::compare(lhs, rhs) == 0;
    }

    bool operator!=(const DataToken& lhs, const DataToken& rhs)
    {
        return !(lhs == rhs);
    }

    bool operator<(const DataToken& lhs, const DataToken& rhs)
    {
        if (lhs.type == DataType::_NULL || rhs.type == DataType::_NULL) return false;
        if (DataToken::common_type(lhs.type, rhs.type) == DataType::UNDEFINED)
            return static_cast<int>(lhs.type) < static_cast<int>(rhs.type);
        return DataToken::compare(lhs, rhs) < 0;
    }

    bool operator<=(const DataToken& lhs, const DataToken& rhs) { return !(rhs < lhs); }
    bool operator>(const DataToken& lhs, const DataToken& rhs)  { return rhs < lhs; }
    bool operator>=(const DataToken& lhs, const DataToken& rhs) { return !(lhs < rhs); }

} // namespace types
