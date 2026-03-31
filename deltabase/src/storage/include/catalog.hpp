//
// Created by poproshaikin on 3/28/26.
//

#ifndef DELTABASE_CATALOG_HPP
#define DELTABASE_CATALOG_HPP

#include "io_manager.hpp"

#include <unordered_map>

namespace storage
{
    class CatalogCache
    {
        IIOManager& io_;
        std::unordered_map<types::Uuid, types::MetaTable> tables_;
        std::unordered_map<types::Uuid, types::MetaSchema> schemas_;

    public:
        explicit CatalogCache(IIOManager& io);
        ~CatalogCache();

        void
        hydrate();

        void
        flush();

        void
        put(types::MetaTable table);

        void
        put(types::MetaSchema schema);

        types::MetaTable*
        get_table(const types::Uuid& id);

        types::MetaTable*
        get_table(const std::string& name, const types::Uuid& schema_id);

        void
        save_table(const types::MetaTable& mt);

        types::MetaSchema*
        get_schema(const types::Uuid& id);

        types::MetaSchema*
        get_schema(const std::string& name);
        bool
        exists_schema(const std::string& name);
        void
        save_schema(const types::MetaSchema& ms);
    };
} // namespace storage

#endif // DELTABASE_CATALOG_HPP
