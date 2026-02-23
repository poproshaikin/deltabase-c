//
// Created by poproshaikin on 13.11.25.
//

#include "include/data_token.hpp"

#include <cstring>
#include <stdexcept>

namespace types
{
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
        default:
            throw std::invalid_argument(
                "Cannot convert SQL token to a data token: invalid value type: " + sql_token.value);
        }
    }

    DataToken::DataToken(const Bytes& bytes, DataType type)
        : bytes(bytes), type(type)
    {
    }
}