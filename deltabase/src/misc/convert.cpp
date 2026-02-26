//
// Created by poproshaikin on 13.11.25.
//

#include "include/convert.hpp"

#include "../sql/include/dictionary.hpp"
#include "../types/include/data_token.hpp"

namespace misc
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

    OutputSchema
    convert(const MetaTable& meta)
    {
        OutputSchema schema;
        schema.reserve(meta.columns.size());

        for (const auto& column : meta.columns)
            schema.emplace_back(column.name, column.type);

        return schema;
    }

    MetaColumn
    convert(const ColumnDefinition& column_def)
    {
        MetaColumn column;
        column.name = column_def.name.value;
        column.type = convert_to_dt(column_def.type);
        column.flags = convert_to_mcf(column_def.constraints);
        return column;
    }

    DataType
    convert_to_dt(const SqlToken& token)
    {
        if (!token.is_keyword())
            throw std::invalid_argument(
                "convert_to_dt: cannot convert SQL token to data type: token is not a keyword"
            );

        auto keyword = token.get_detail<sql::SqlKeyword>();
        auto type = sql::to_data_type(keyword);

        if (type == DataType::UNDEFINED)
            throw std::runtime_error(
                "convert_to_dt: cannot convert SQL token to a data type: unknown type keyword '"
                + token.value + "'"
            );

        return type;
    }

    MetaColumnFlags
    convert_to_mcf(const std::vector<SqlToken>& tokens)
    {
        MetaColumnFlags flags = MetaColumnFlags::NONE;

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            if (!tokens[i].is_keyword())
                continue;

            auto keyword = tokens[i].get_detail<sql::SqlKeyword>();

            switch (keyword)
            {
            case SqlKeyword::PRIMARY:
                flags = flags | MetaColumnFlags::PK;
                break;
            case SqlKeyword::UNIQUE:
                flags = flags | MetaColumnFlags::UN;
                break;
            case SqlKeyword::AUTOINCREMENT:
                flags = flags | MetaColumnFlags::AI;
                break;
            case SqlKeyword::NOT:
                if (i + 1 < tokens.size() &&
                    tokens[i + 1].is_keyword() &&
                    tokens[i + 1].get_detail<SqlKeyword>() == SqlKeyword::_NULL)
                {
                    flags = flags | MetaColumnFlags::NN;
                }
                break;
            default:
                break;
            }
        }

        return flags;
    }

    DataRow
    convert(const ValuesExpr& values_expr)
    {
        std::vector<DataToken> tokens(values_expr.values.size());

        for (const auto& sql_token : values_expr.values)
        {
            tokens.emplace_back(DataToken(sql_token));
        }

        DataRow row;
        row.id = 0;
        row.flags = DataRowFlags::NONE;
        row.tokens = std::move(tokens);
        return row;
    }
} // namespace misc