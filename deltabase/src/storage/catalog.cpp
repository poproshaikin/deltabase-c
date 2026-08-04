//
// Created by poproshaikin on 3/28/26.
//

#include "include/catalog.hpp"

#include <limits>
#include <ranges>

namespace storage
{
    CatalogCache::CatalogCache(IIOManager& io) : io_(io)
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
        std::lock_guard lock(mutex_);
        hydrate_impl();
    }

    void
    CatalogCache::hydrate_impl()
    {
        tables_.clear();
        schemas_.clear();
        sequences_.clear();

        for (auto&& table : io_.read_tables_meta())
            tables_.emplace(table.id, std::move(table));

        for (auto&& schema : io_.read_schemas_meta())
            schemas_.emplace(schema.id, std::move(schema));

        for (auto&& sequence : io_.read_sequences())
            sequences_.emplace(sequence.id, std::move(sequence));
    }

    void
    CatalogCache::flush()
    {
        flush_impl();
    }

    void
    CatalogCache::flush_impl()
    {
        flush_impl(std::numeric_limits<types::LSN>::max());
    }

    void
    CatalogCache::flush(types::LSN max_lsn)
    {
        flush_impl(max_lsn);
    }

    void
    CatalogCache::flush_impl(types::LSN max_lsn)
    {
        // Collects eligible entries under the lock, writes them to disk with the
        // lock released (fsync can take milliseconds -- must never block other
        // threads for that long), then briefly re-locks to clear the dirty flag
        // only for entries that weren't mutated again in the meantime.
        auto flush_container = [this, max_lsn]<typename TValue, typename Writer>(
            std::unordered_map<types::UUID, CatalogEntry<TValue>>& container, Writer&& writer)
        {
            std::vector<std::pair<types::UUID, TValue>> to_write;
            {
                std::lock_guard lock(mutex_);
                for (auto& [id, entry] : container)
                {
                    if (!entry.dirty || entry.last_lsn > max_lsn)
                        continue;
                    to_write.emplace_back(id, entry.value);
                }
            }

            for (const auto& [id, value] : to_write)
                writer(value);

            {
                std::lock_guard lock(mutex_);
                for (const auto& [id, value] : to_write)
                {
                    auto it = container.find(id);
                    if (it != container.end() && it->second.last_lsn <= max_lsn)
                        it->second.dirty = false;
                }
            }
        };

        flush_container(schemas_, [this](const types::MetaSchema& s) { io_.write_ms(s, true); });
        flush_container(tables_, [this](const types::MetaTable& t) { io_.write_mt(t, true); });
        flush_container(sequences_, [this](const types::MetaSequence& s) { io_.write_seq(s, true); });
    }

    void
    CatalogCache::put(types::MetaTable table, types::LSN last_lsn)
    {
        std::lock_guard lock(mutex_);
        put_impl(std::move(table), last_lsn);
    }

    void
    CatalogCache::put_impl(types::MetaTable table, types::LSN last_lsn)
    {
        auto id = table.id;
        auto it = tables_.find(id);
        if (it == tables_.end())
            it = tables_.emplace(id, std::move(table)).first;
        else
            it->second.value = std::move(table);
        it->second.dirty = true;
        it->second.last_lsn = last_lsn;
    }

    void
    CatalogCache::put(types::MetaSchema schema, types::LSN last_lsn)
    {
        std::lock_guard lock(mutex_);
        put_impl(std::move(schema), last_lsn);
    }

    void
    CatalogCache::put_impl(types::MetaSchema schema, types::LSN last_lsn)
    {
        auto id = schema.id;
        auto it = schemas_.find(id);
        if (it == schemas_.end())
            it = schemas_.emplace(id, std::move(schema)).first;
        else
            it->second.value = std::move(schema);
        it->second.dirty = true;
        it->second.last_lsn = last_lsn;
    }

    void
    CatalogCache::put(types::MetaSequence sequence, types::LSN last_lsn)
    {
        std::lock_guard lock(mutex_);
        put_impl(std::move(sequence), last_lsn);
    }

    void
    CatalogCache::put_impl(types::MetaSequence sequence, types::LSN last_lsn)
    {
        auto id = sequence.id;
        auto it = sequences_.find(id);
        if (it == sequences_.end())
            it = sequences_.emplace(id, std::move(sequence)).first;
        else
            it->second.value = std::move(sequence);
        it->second.dirty = true;
        it->second.last_lsn = last_lsn;
    }

    void
    CatalogCache::mark_dirty(const types::MetaTable* table, types::LSN last_lsn)
    {
        std::lock_guard lock(mutex_);
        mark_dirty_impl(table, last_lsn);
    }

    void
    CatalogCache::mark_dirty_impl(const types::MetaTable* table, types::LSN last_lsn)
    {
        auto it = tables_.find(table->id);
        if (it != tables_.end())
        {
            it->second.dirty = true;
            it->second.last_lsn = last_lsn;
        }
    }

    void
    CatalogCache::mark_dirty(const types::MetaSchema* schema, types::LSN last_lsn)
    {
        std::lock_guard lock(mutex_);
        mark_dirty_impl(schema, last_lsn);
    }

    void
    CatalogCache::mark_dirty_impl(const types::MetaSchema* schema, types::LSN last_lsn)
    {
        auto it = schemas_.find(schema->id);
        if (it != schemas_.end())
        {
            it->second.dirty = true;
            it->second.last_lsn = last_lsn;
        }
    }

    void
    CatalogCache::mark_dirty(const types::MetaSequence* sequence, types::LSN last_lsn)
    {
        std::lock_guard lock(mutex_);
        mark_dirty_impl(sequence, last_lsn);
    }

    void
    CatalogCache::mark_dirty_impl(const types::MetaSequence* sequence, types::LSN last_lsn)
    {
        auto it = sequences_.find(sequence->id);
        if (it != sequences_.end())
        {
            it->second.dirty = true;
            it->second.last_lsn = last_lsn;
        }
    }

    types::MetaTable*
    CatalogCache::get_table(const types::UUID& id)
    {
        std::lock_guard lock(mutex_);
        return get_table_impl(id);
    }

    types::MetaTable*
    CatalogCache::get_table_impl(const types::UUID& id)
    {
        auto it = tables_.find(id);
        return it == tables_.end() ? nullptr : &it->second.value;
    }

    const types::MetaTable*
    CatalogCache::get_table(const types::UUID& id) const
    {
        std::lock_guard lock(mutex_);
        return get_table_impl(id);
    }

    const types::MetaTable*
    CatalogCache::get_table_impl(const types::UUID& id) const
    {
        auto it = tables_.find(id);
        return it == tables_.end() ? nullptr : &it->second.value;
    }

    types::MetaTable*
    CatalogCache::get_table(const std::string& name, const types::UUID& schema_id)
    {
        std::lock_guard lock(mutex_);
        return get_table_impl(name, schema_id);
    }

    types::MetaTable*
    CatalogCache::get_table_impl(const std::string& name, const types::UUID& schema_id)
    {
        for (auto& [_, table] : tables_)
        {
            if (table.value.name == name && table.value.schema_id == schema_id)
                return &table.value;
        }

        return nullptr;
    }

    types::MetaTable*
    CatalogCache::save_table(types::MetaTable&& mt, types::LSN last_lsn)
    {
        std::lock_guard lock(mutex_);
        return save_table_impl(std::move(mt), last_lsn);
    }

    types::MetaTable*
    CatalogCache::save_table_impl(types::MetaTable&& mt, types::LSN last_lsn)
    {
        auto id = mt.id;
        auto it = tables_.find(id);
        if (it == tables_.end())
            it = tables_.emplace(id, std::move(mt)).first;
        else
            it->second.value = std::move(mt);
        it->second.dirty = true;
        it->second.last_lsn = last_lsn;
        return &it->second.value;
    }

    void
    CatalogCache::delete_table(const types::UUID& table_id)
    {
        std::lock_guard lock(mutex_);
        delete_table_impl(table_id);
    }

    void
    CatalogCache::delete_table_impl(const types::UUID& table_id)
    {
        tables_.erase(table_id);
    }

    types::MetaSchema*
    CatalogCache::get_schema(const types::UUID& id)
    {
        std::lock_guard lock(mutex_);
        return get_schema_impl(id);
    }

    types::MetaSchema*
    CatalogCache::get_schema_impl(const types::UUID& id)
    {
        const auto it = schemas_.find(id);
        return it == schemas_.end() ? nullptr : &it->second.value;
    }

    types::MetaSchema*
    CatalogCache::get_schema(const std::string& name)
    {
        std::lock_guard lock(mutex_);
        return get_schema_impl(name);
    }

    types::MetaSchema*
    CatalogCache::get_schema_impl(const std::string& name)
    {
        for (auto& [_, schema] : schemas_)
        {
            if (schema.value.name == name)
                return &schema.value;
        }

        return nullptr;
    }

    void
    CatalogCache::delete_schema(const types::UUID& schema_id)
    {
        std::lock_guard lock(mutex_);
        delete_schema_impl(schema_id);
    }

    void
    CatalogCache::delete_schema_impl(const types::UUID& schema_id)
    {
        schemas_.erase(schema_id);
    }

    void
    CatalogCache::delete_sequence(const types::UUID& sequence_id)
    {
        std::lock_guard lock(mutex_);
        delete_sequence_impl(sequence_id);
    }

    void
    CatalogCache::delete_sequence_impl(const types::UUID& sequence_id)
    {
        sequences_.erase(sequence_id);
    }

    types::MetaSequence*
    CatalogCache::get_sequence(const types::UUID& id)
    {
        std::lock_guard lock(mutex_);
        return get_sequence_impl(id);
    }

    types::MetaSequence*
    CatalogCache::get_sequence_impl(const types::UUID& id)
    {
        const auto it = sequences_.find(id);
        return it == sequences_.end() ? nullptr : &it->second.value;
    }

    bool
    CatalogCache::exists_schema(const std::string& name)
    {
        std::lock_guard lock(mutex_);
        return exists_schema_impl(name);
    }

    bool
    CatalogCache::exists_schema_impl(const std::string& name)
    {
        return get_schema_impl(name) != nullptr;
    }

    types::MetaSchema*
    CatalogCache::save_schema(const types::MetaSchema& ms, types::LSN last_lsn)
    {
        std::lock_guard lock(mutex_);
        return save_schema_impl(ms, last_lsn);
    }

    types::MetaSchema*
    CatalogCache::save_schema_impl(const types::MetaSchema& ms, types::LSN last_lsn)
    {
        auto it = schemas_.find(ms.id);
        if (it == schemas_.end())
            it = schemas_.emplace(ms.id, types::MetaSchema(ms)).first;
        else
            it->second.value = ms;
        it->second.dirty = true;
        it->second.last_lsn = last_lsn;
        return &it->second.value;
    }

    std::vector<types::MetaTable*>
    CatalogCache::get_all_tables()
    {
        std::lock_guard lock(mutex_);
        return get_all_tables_impl();
    }

    std::vector<types::MetaTable*>
    CatalogCache::get_all_tables_impl()
    {
        std::vector<types::MetaTable*> tables;
        tables.reserve(tables_.size());

        for (auto& table : tables_ | std::views::values)
            tables.push_back(&table.value);

        return tables;
    }

    std::vector<types::MetaTable*>
    CatalogCache::get_all_tables(const types::SchemaId& schema_id)
    {
        std::lock_guard lock(mutex_);
        return get_all_tables_impl(schema_id);
    }

    std::vector<types::MetaTable*>
    CatalogCache::get_all_tables_impl(const types::SchemaId& schema_id)
    {
        std::vector<types::MetaTable*> tables;
        tables.reserve(tables_.size());

        for (auto& table : tables_ | std::views::values)
            if (table.value.schema_id == schema_id)
                tables.push_back(&table.value);

        return tables;
    }

    std::vector<types::MetaSchema*>
    CatalogCache::get_all_schemas()
    {
        std::lock_guard lock(mutex_);
        return get_all_schemas_impl();
    }

    std::vector<types::MetaSchema*>
    CatalogCache::get_all_schemas_impl()
    {
        std::vector<types::MetaSchema*> schemas;
        schemas.reserve(schemas_.size());

        for (auto& schema : schemas_ | std::views::values)
            schemas.push_back(&schema.value);

        return schemas;
    }
} // namespace storage
