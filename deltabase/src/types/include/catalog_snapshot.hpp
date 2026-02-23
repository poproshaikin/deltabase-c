//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_CATALOG_SNAPSHOT_HPP
#define DELTABASE_CATALOG_SNAPSHOT_HPP
#include "meta_table.hpp"
#include "meta_schema.hpp"
#include "uuid.hpp"

#include <cstring>
#include <unordered_map>

namespace types
{
    struct CatalogSnapshot
    {
        static inline uint64_t last_version_ = 0;

        template <typename T>
        struct Entry
        {
            static inline uint64_t last_version_ = 0;

            uint64_t version;
            T value;

            Entry(T value) : version(last_version_++), value(value)
            {
            }

        };

        uint64_t version;
        std::unordered_map<Uuid, Entry<MetaTable> > tables;
        std::unordered_map<Uuid, Entry<MetaSchema> > schemas;

        CatalogSnapshot(
            const std::unordered_map<Uuid, MetaTable>& tables,
            const std::unordered_map<Uuid, MetaSchema>& schemas
        );

        MetaSchema
        get_schema(const std::string& schema_name);

        MetaTable
        get_table(const std::string& table_name, const std::string& schema_name);

        bool
        operator==(const CatalogSnapshot& other) const;

        bool
        operator!=(const CatalogSnapshot& other) const;

    };
}

#endif //DELTABASE_CATALOG_SNAPSHOT_HPP