//
// Created by poproshaikin on 02.12.25.
//

#include "include/meta_table.hpp"

#include <stdexcept>

namespace types
{
    MetaTable::MetaTable()
        : id(Uuid::null()), schema_id(Uuid::null())
    {
    }

    DataRow
    MetaTable::make_row(
        const std::optional<std::vector<std::string>>& cols, const std::vector<DataToken>& row
    )
    {
        const DataToken null_token(Bytes{}, DataType::_NULL);

        DataRow data_row;
        data_row.id = last_rid++;

        total_rows++;
        live_rows++;

        // Always reorder tokens according to schema and fill missing values with NULL tokens
        std::vector<DataToken> reordered_tokens(columns.size(), null_token);

        if (!cols.has_value())
        {
            // No column names provided - map row tokens directly by index
            for (size_t i = 0; i < row.size() && i < columns.size(); ++i)
            {
                reordered_tokens[i] = row[i];
            }
        }
        else
        {
            // Column names provided - map tokens according to schema
            for (size_t i = 0; i < cols->size(); ++i)
            {
                int64_t col_idx = get_column_idx((*cols)[i]);
                if (col_idx != -1 && i < row.size())
                {
                    reordered_tokens[col_idx] = row[i];
                }
            }
        }

        data_row.tokens = reordered_tokens;
        return data_row;
    }

    bool
    MetaTable::has_column(const std::string& col_name) const
    {
        for (const auto& column : columns)
        {
            if (column.name == col_name)
                return true;
        }
        return false;
    }

    const MetaColumn&
    MetaTable::get_column(const std::string& col_name) const
    {
        for (const auto& column : columns)
        {
            if (column.name == col_name)
                return column;
        }
        throw std::runtime_error("MetaTable::get_column: column '" + col_name + "' not found");
    }

    const MetaColumn&
    MetaTable::get_column(const int64_t& col_pos) const
    {
        if (col_pos < 0 || col_pos >= columns.size())
            throw std::runtime_error("MetaTable::get_column: column at pos '" + std::to_string(col_pos) + "' not found");

        return columns[col_pos];
    }

    const MetaColumn&
    MetaTable::get_column(const ColumnId& col_id) const
    {
        for (const auto& column : columns)
        {
            if (column.id == col_id)
                return column;
        }
        throw std::runtime_error("MetaTable::get_column: column '" + col_id.to_string() + "' not found");
    }

    int64_t
    MetaTable::get_column_idx(const std::string& col_name) const
    {
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i].name == col_name)
                return static_cast<int64_t>(i);
        }
        return -1;
    }

    int64_t
    MetaTable::get_column_idx(const ColumnId& col_id) const
    {
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i].id == col_id)
                return static_cast<int64_t>(i);
        }

        throw std::runtime_error("MetaTable::get_column_idx: column " + col_id.to_string() + " doesnt exist");
    }
} // namespace types
