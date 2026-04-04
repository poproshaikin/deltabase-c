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
