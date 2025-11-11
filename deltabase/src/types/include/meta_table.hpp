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
        std::string id;
        std::string schema_id;
        std::string name;
        std::vector<MetaColumn> columns;
        uint64_t last_rid;

        std::vector<std::string> pages_ids; // DO NOT SERIALIZE, RAM ONLY

        MetaTable();
        MetaTable(MetaTable&& other) = default;
        // Restrict copying
        MetaTable(const MetaTable& other) = delete;

        bool
        has_column(const std::string& name) const;

        const MetaColumn&
        get_column(const std::string& name) const;

        bool
        compare_content(const MetaTable& other) const;

        static bool
        try_deserialize(const Bytes& bytes, MetaTable& out);
        Bytes
        serialize() const;
    };
}

#endif //DELTABASE_META_TABLE_HPP