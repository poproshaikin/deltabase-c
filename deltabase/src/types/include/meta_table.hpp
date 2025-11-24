//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_META_TABLE_HPP
#define DELTABASE_META_TABLE_HPP
#include "meta_column.hpp"

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
        uint64_t last_rid;
        std::vector<std::string> pages_ids; // DO NOT SERIALIZE, RAM ONLY

        MetaTable();

        bool
        has_column(const std::string& name) const;

        const MetaColumn&
        get_column(const std::string& name) const;

        // -1 if not found
        int64_t
        get_column_idx(const std::string& name) const;
    };
}

#endif //DELTABASE_META_TABLE_HPP