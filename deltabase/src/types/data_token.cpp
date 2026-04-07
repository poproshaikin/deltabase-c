//
// Created by poproshaikin on 13.11.25.
//

#include "include/data_token.hpp"

#include "../misc/include/logger.hpp"

#include <cstring>
#include <stdexcept>

namespace types
{
    namespace
    {
        bool
        is_same_type(const DataToken& lhs, const DataToken& rhs)
        {
            return lhs.type == rhs.type;
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
            bytes.resize(sql_token.value.size());
            std::memcpy(bytes.data(), sql_token.value.data(), sql_token.value.size());
            type = DataType::STRING;
            break;
        }
        case SqlLiteral::BOOL:
        {
            bool value = sql_token.value == "true" || sql_token.value == "1";
            bytes.resize(1);
            bytes[0] = value ? 1 : 0;
            type = DataType::BOOL;
            break;
        }
        case SqlLiteral::CHAR:
        {
            bytes.resize(1);
            bytes[0] = static_cast<unsigned char>(sql_token.value[0]);
            type = DataType::CHAR;
            break;
        }
        case SqlLiteral::NULL_:
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

    bool
    operator==(const DataToken& lhs, const DataToken& rhs)
    {
        if (!is_same_type(lhs, rhs))
            return false;

        switch (lhs.type)
        {
        case DataType::_NULL:
            return true;
        case DataType::INTEGER:
            return lhs.as<int>() == rhs.as<int>();
        case DataType::REAL:
            return lhs.as<double>() == rhs.as<double>();
        case DataType::CHAR:
            return lhs.as<char>() == rhs.as<char>();
        case DataType::BOOL:
            return lhs.as<bool>() == rhs.as<bool>();
        case DataType::STRING:
            return lhs.as<std::string>() == rhs.as<std::string>();
        default:
        {
            misc::Logger::warn(
                "Missing comparison operator for data token type " +
                std::to_string(static_cast<int>(lhs.type))
            );
            return lhs.bytes == rhs.bytes;
        }
        }
    }

    bool
    operator!=(const DataToken& lhs, const DataToken& rhs)
    {
        return !(lhs == rhs);
    }

    bool
    operator<(const DataToken& lhs, const DataToken& rhs)
    {
        if (!is_same_type(lhs, rhs))
            return static_cast<uint64_t>(lhs.type) < static_cast<uint64_t>(rhs.type);

        switch (lhs.type)
        {
        case DataType::INTEGER:
            return lhs.as<int>() < rhs.as<int>();
        case DataType::REAL:
            return lhs.as<double>() < rhs.as<double>();
        case DataType::CHAR:
            return lhs.as<char>() < rhs.as<char>();
        case DataType::BOOL:
            return lhs.as<bool>() < rhs.as<bool>();
        case DataType::STRING:
            return lhs.as<std::string>() < rhs.as<std::string>();
        default:
        {
            misc::Logger::warn(
                "Missing comparison operator for data token type " +
                std::to_string(static_cast<int>(lhs.type))
            );
            return lhs.bytes < rhs.bytes;
        }
        }
    }

    bool
    operator<=(const DataToken& lhs, const DataToken& rhs)
    {
        return lhs < rhs || lhs == rhs;
    }

    bool
    operator>(const DataToken& lhs, const DataToken& rhs)
    {
        return rhs < lhs;
    }

    bool
    operator>=(const DataToken& lhs, const DataToken& rhs)
    {
        return !(lhs < rhs);
    }
} // namespace types