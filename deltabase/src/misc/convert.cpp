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
        column.constraints.clear();
        for (const auto& constraint : column_def.constraints)
            column.constraints.emplace_back(convert(constraint));

        return column;
    }

    ColumnConstraint
    convert(const Constraint& constraint)
    {
        if (std::holds_alternative<NotNullConstraint>(constraint))
            return MetaNotNullConstraint{};

        if (const auto* default_constraint = std::get_if<DefaultConstraint>(&constraint))
            return MetaDefaultConstraint{ DataToken(default_constraint->value) };

        throw std::runtime_error("convert: unsupported column constraint");
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