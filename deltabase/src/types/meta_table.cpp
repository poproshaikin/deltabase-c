//
// Created by poproshaikin on 02.12.25.
//

#include "include/meta_table.hpp"

#include <stdexcept>

namespace
{
    bool
    has_not_null_constraint(const types::MetaColumn& column)
    {
        for (const auto& constraint : column.constraints)
        {
            if (std::holds_alternative<types::MetaNotNullConstraint>(constraint))
                return true;
        }

        return false;
    }

    const types::MetaDefaultConstraint*
    get_default_constraint(const types::MetaColumn& column)
    {
        for (const auto& constraint : column.constraints)
        {
            if (const auto* default_constraint = std::get_if<types::MetaDefaultConstraint>(&constraint))
                return default_constraint;
        }

        return nullptr;
    }
}

namespace types
{
    MetaTable::MetaTable()
        : id(UUID::null()), schema_id(UUID::null())
    {
    }

    DataRow
    MetaTable::make_row(const std::vector<DataToken>& normalized_row)
    {
        DataRow data_row;
        data_row.id = last_rid++;
        total_rows++;
        live_rows++;
        data_row.tokens = normalized_row;
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

    bool
    MetaTable::is_unique(const std::string& col_name) const
    {
        for (const auto& index : indexes)
        {
            auto column = get_column(index.column_id);
            if (column.name == col_name && index.is_unique)
                return true;
        }

        return false;
    }

    std::vector<MetaIndex*>
    MetaTable::get_indexes(const std::string& col_name, bool only_unique)
    {
        std::vector<MetaIndex*> result;
        for (auto& index : indexes)
        {
            auto column = get_column(index.column_id);
            if (column.name == col_name && (!only_unique || index.is_unique))
                result.push_back(&index);
        }
        return result;
    }

    std::vector<const MetaIndex*>
    MetaTable::get_indexes(const std::string& col_name, bool only_unique) const
    {
        std::vector<const MetaIndex*> result;
        for (const auto& index : indexes)
        {
            auto column = get_column(index.column_id);
            if (column.name == col_name && (!only_unique || index.is_unique))
                result.push_back(&index);
        }
        return result;
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
