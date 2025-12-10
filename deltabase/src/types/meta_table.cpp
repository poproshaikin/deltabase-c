//
// Created by poproshaikin on 02.12.25.
//

#include "include/meta_table.hpp"
#include <stdexcept>

namespace types
{
    MetaTable::MetaTable()
        : id(), schema_id(), name(), columns(), last_rid(0)
    {
    }

    bool
    MetaTable::has_column(const std::string& name) const
    {
        for (const auto& column : columns)
        {
            if (column.name == name)
                return true;
        }
        return false;
    }

    const MetaColumn&
    MetaTable::get_column(const std::string& name) const
    {
        for (const auto& column : columns)
        {
            if (column.name == name)
                return column;
        }
        throw std::runtime_error("MetaTable::get_column: column '" + name + "' not found");
    }

    int64_t
    MetaTable::get_column_idx(const std::string& name) const
    {
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i].name == name)
                return static_cast<int64_t>(i);
        }
        return -1;
    }
}
