//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_META_TABLE_HPP
#define DELTABASE_META_TABLE_HPP
#include "meta_column.hpp"

#include "data_row_update.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace types
{
    struct MetaTable
    {
        Uuid id;
        Uuid schema_id;
        std::string name;
        std::vector<MetaColumn> columns;
        RowId last_rid;

        MetaTable();

        bool
        has_column(const std::string& col_name) const;

        const MetaColumn&
        get_column(const std::string& col_name) const;

        // -1 if not found
        int64_t
        get_column_idx(const std::string& col_name) const;

        int64_t
        get_column_idx(const ColumnId& col_id) const;
    };
}

#endif //DELTABASE_META_TABLE_HPP