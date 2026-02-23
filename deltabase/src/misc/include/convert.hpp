//
// Created by poproshaikin on 13.11.25.
//

#ifndef DELTABASE_CONVERT_HPP
#define DELTABASE_CONVERT_HPP
#include "../../types/include/data_table.hpp"

#include <cstring>

namespace misc
{
    template <typename T>
    types::Bytes
    convert(const T& value) requires(std::is_trivially_copyable_v<T>)
    {
        types::Bytes bytes(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));
        return bytes;
    }

    types::Bytes
    convert(const std::string& value);

    types::Bytes
    stob(const std::string& value);

    types::OutputSchema
    convert(const types::MetaTable& meta);

    types::MetaColumn
    convert(const types::ColumnDefinition& column_def);

    types::DataType
    convert_to_dt(const types::SqlToken& token);

    types::MetaColumnFlags
    convert_to_mcf(const std::vector<types::SqlToken>& tokens);

    types::DataRow
    convert(const types::ValuesExpr& values_expr);
}

#endif //DELTABASE_CONVERT_HPP