//
// Created by poproshaikin on 13.11.25.
//

#include "include/convert.hpp"
#include "../types/include/data_token.hpp"

namespace utils
{
    using namespace types;

    Bytes
    convert(const std::string& value)
    {
        Bytes bytes(value.size());
        std::memcpy(bytes.data(), value.data(), value.size());
        return bytes;
    }

    Bytes
    stob(const std::string& literal)
    {
        bool value = literal == "true" || literal == "1";
        Bytes bytes(1);
        bytes[0] = value;
        return bytes;
    }

    DataToken
    convert(const SqlToken& sql_token)
    {
        if (!sql_token.is_literal())
            throw std::invalid_argument("Cannot convert non-literal SQL token to a data token");

        auto literal_type = sql_token.get_detail<SqlLiteral>();

        Bytes bytes;
        auto val_type = DataType::UNDEFINED;

        switch (literal_type)
        {
        case SqlLiteral::INTEGER:
        {
            int temp = std::stoi(sql_token.value);
            bytes = misc::convert(temp);
            val_type = DataType::INTEGER;
            break;
        }
        case SqlLiteral::REAL:
        {
            double temp = std::stoi(sql_token.value);
            bytes = misc::convert(temp);
            val_type = DataType::REAL;
            break;
        }
        case SqlLiteral::STRING:
        {
            bytes = misc::convert(sql_token.value);
            val_type = DataType::STRING;
            break;
        }
        case SqlLiteral::BOOL:
        {
            bytes = misc::stob(sql_token.value);
            val_type = DataType::BOOL;
            break;
        }
        case SqlLiteral::CHAR:
        {
            bytes = misc::convert(sql_token.value[0]);
            val_type = DataType::CHAR;
            break;
        }
        default:
            throw std::invalid_argument("Cannot convert SQL token to a data token: invalid value type: " + sql_token.value);
        }

        return DataToken(bytes, val_type);
    }

    OutputSchema
    convert(const MetaTable& meta)
    {
        OutputSchema schema;
        schema.reserve(meta.columns.size());

        for (const auto& column : meta.columns)
            schema.emplace_back(column.name, column.type);

        return schema;
    }
}