//
// Created by poproshaikin on 3/28/26.
//

#include "include/catalog.hpp"

namespace storage
{
    CatalogCache::CatalogCache(storage::IIOManager& io) : io_(io)
    {
    }

    CatalogCache::~CatalogCache()
    {
        try
        {
            flush();
        }
        catch (...)
        {
            // Avoid throwing from destructor.
        }
    }

    void
    CatalogCache::hydrate()
    {
        tables_.clear();
        schemas_.clear();

        for (auto&& table : io_.read_tables_meta())
            tables_.emplace(table.id, std::move(table));

        for (auto&& schema : io_.read_schemas_meta())
            schemas_.emplace(schema.id, std::move(schema));
    }

    void
    CatalogCache::flush()
    {
        for (const auto& [_, schema] : schemas_)
            io_.write_ms(schema, true);

        for (const auto& [_, table] : tables_)
            io_.write_mt(table, true);
    }

    void
    CatalogCache::put(types::MetaTable table)
    {
        tables_[table.id] = std::move(table);
    }

    void
    CatalogCache::put(types::MetaSchema schema)
    {
        schemas_[schema.id] = std::move(schema);
    }

    types::MetaTable*
    CatalogCache::get_table(const types::Uuid& id)
    {
        auto it = tables_.find(id);
        return it == tables_.end() ? nullptr : &it->second;
    }

    types::MetaTable*
    CatalogCache::get_table(const std::string& name, const types::Uuid& schema_id)
    {
        for (auto& [_, table] : tables_)
        {
            if (table.name == name && table.schema_id == schema_id)
                return &table;
        }

        return nullptr;
    }

    void
    CatalogCache::save_table(const types::MetaTable& mt)
    {
        auto it = tables_.find(mt.id);
        if (it == tables_.end())
        {
            tables_.emplace(mt.id, mt);
        }
        else
        {
            it->second = mt;
        }
    }

    types::MetaSchema*
    CatalogCache::get_schema(const types::Uuid& id)
    {
        const auto it = schemas_.find(id);
        return it == schemas_.end() ? nullptr : &it->second;
    }

    types::MetaSchema*
    CatalogCache::get_schema(const std::string& name)
    {
        for (auto& [_, schema] : schemas_)
        {
            if (schema.name == name)
                return &schema;
        }

        return nullptr;
    }
    bool
    CatalogCache::exists_schema(const std::string& name)
    {
        return get_schema(name) != nullptr;
    }
    void
    CatalogCache::save_schema(const types::MetaSchema& ms)
    {
        auto it = schemas_.find(ms.id);
        if (it == schemas_.end())
        {
            schemas_.emplace(ms.id, ms);
        }
        else
        {
            it->second = ms;
        }
    }
} // namespace storage
