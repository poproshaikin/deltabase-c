//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_META_TABLE_HPP
#define DELTABASE_META_TABLE_HPP
#include "meta_column.hpp"

#include "data_row_update.hpp"
#include "meta_index.hpp"
#include "meta_schema.hpp"
#include "table_id.hpp"

#include <string>
#include <vector>

namespace types
{
    struct MetaTable
    {
        TableId id;
        SchemaId schema_id;
        std::string name;
        std::vector<MetaColumn> columns;
        std::vector<MetaIndex> indexes;
        RowId last_rid;

        MetaTable();

        bool
        has_column(const std::string& col_name) const;

        const MetaColumn&
        get_column(const std::string& col_name) const;

        const MetaColumn&
        get_column(const int64_t& col_pos) const;

        // -1 if not found
        int64_t
        get_column_idx(const std::string& col_name) const;

        int64_t
        get_column_idx(const ColumnId& col_id) const;
    };
}

#endif //DELTABASE_META_TABLE_HPP