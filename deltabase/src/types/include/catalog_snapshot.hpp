//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_CATALOG_SNAPSHOT_HPP
#define DELTABASE_CATALOG_SNAPSHOT_HPP
#include "meta_table.hpp"
#include "meta_schema.hpp"
#include "uuid.hpp"

#include <unordered_map>

namespace types
{
    struct CatalogSnapshot
    {
        template <typename T>
        struct Entry
        {
            uint64_t version;
            T value;

            Entry(T value) : version(last_version_++), value(value)
            {
            }

        private:
            static inline uint64_t last_version_ = 0;
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

    private:
        static inline uint64_t last_version_ = 0;
    };
}

#endif //DELTABASE_CATALOG_SNAPSHOT_HPP