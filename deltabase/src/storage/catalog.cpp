//
// Created by poproshaikin on 3/28/26.
//

#include "include/catalog.hpp"

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
        tables_.clear();
        schemas_.clear();
        sequences_.clear();

        for (auto&& table : io_.read_tables_meta())
            tables_.emplace(table.id, std::move(table));

        for (auto&& schema : io_.read_schemas_meta())
            schemas_.emplace(schema.id, std::move(schema));

        for (auto&& sequence : io_.read_sequences())
            sequences_.emplace(sequence.id, sequence);
    }

    void
    CatalogCache::flush()
    {
        for (const auto& [_, schema] : schemas_)
            io_.write_ms(schema, true);

        for (const auto& [_, table] : tables_)
            io_.write_mt(table, true);

        for (const auto& [_, sequence] : sequences_)
            io_.write_seq(sequence, true);
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

    void
    CatalogCache::put(types::MetaSequence sequence)
    {
        sequences_[sequence.id] = std::move(sequence);
    }

    void
    CatalogCache::put(types::MetaTable table, const types::UUID& txn_id)
    {
        txn_deltas_[txn_id].added_tables.push_back(table.id);
        put(std::move(table));
    }

    void
    CatalogCache::put(types::MetaSchema schema, const types::UUID& txn_id)
    {
        txn_deltas_[txn_id].added_schemas.push_back(schema.id);
        put(std::move(schema));
    }

    void
    CatalogCache::put(types::MetaSequence sequence, const types::UUID& txn_id)
    {
        txn_deltas_[txn_id].added_sequences.push_back(sequence.id);
        put(std::move(sequence));
    }

    void
    CatalogCache::commit_txn(const types::UUID& txn_id)
    {
        txn_deltas_.erase(txn_id);
    }

    void
    CatalogCache::rollback_txn(const types::UUID& txn_id)
    {
        auto it = txn_deltas_.find(txn_id);
        if (it == txn_deltas_.end())
            return;

        auto& delta = it->second;

        for (const auto& id : delta.added_tables)    tables_.erase(id);
        for (const auto& id : delta.added_schemas)   schemas_.erase(id);
        for (const auto& id : delta.added_sequences) sequences_.erase(id);

        for (auto& t : delta.removed_tables)         tables_[t.id]  = std::move(t);
        for (auto& s : delta.removed_schemas)        schemas_[s.id] = std::move(s);
        for (auto& s : delta.removed_sequences)      sequences_[s.id] = std::move(s);
        for (auto& t : delta.updated_tables_before)  tables_[t.id]  = std::move(t);

        txn_deltas_.erase(it);
    }

    types::MetaTable*
    CatalogCache::get_table(const types::UUID& id)
    {
        auto it = tables_.find(id);
        return it == tables_.end() ? nullptr : &it->second;
    }

    const types::MetaTable*
    CatalogCache::get_table(const types::UUID& id) const
    {
        auto it = tables_.find(id);
        return it == tables_.end() ? nullptr : &it->second;
    }

    types::MetaTable*
    CatalogCache::get_table(const std::string& name, const types::UUID& schema_id)
    {
        for (auto& [_, table] : tables_)
        {
            if (table.name == name && table.schema_id == schema_id)
                return &table;
        }

        return nullptr;
    }

    void
    CatalogCache::put_or_update(types::MetaTable table, const types::UUID& txn_id)
    {
        auto it = tables_.find(table.id);
        if (it == tables_.end())
        {
            txn_deltas_[txn_id].added_tables.push_back(table.id);
        }
        else
        {
            txn_deltas_[txn_id].updated_tables_before.push_back(it->second);
        }
        tables_[table.id] = std::move(table);
    }

    types::MetaTable*
    CatalogCache::save_table(types::MetaTable&& mt, const types::UUID& txn_id)
    {
        put_or_update(mt, txn_id);
        return &tables_[mt.id];
    }

    void
    CatalogCache::delete_table(const types::UUID& table_id)
    {
        tables_.erase(table_id);
    }

    void
    CatalogCache::delete_table(const types::UUID& table_id, const types::UUID& txn_id)
    {
        auto it = tables_.find(table_id);
        if (it == tables_.end())
            return;

        txn_deltas_[txn_id].removed_tables.push_back(it->second);
        tables_.erase(it);
    }

    types::MetaSchema*
    CatalogCache::get_schema(const types::UUID& id)
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

    void
    CatalogCache::delete_schema(const types::UUID& schema_id, const types::UUID& txn_id)
    {
        auto it = schemas_.find(schema_id);
        if (it == schemas_.end())
            return;

        txn_deltas_[txn_id].removed_schemas.push_back(it->second);
        schemas_.erase(it);
    }

    void
    CatalogCache::delete_sequence(const types::UUID& sequence_id, const types::UUID& txn_id)
    {
        auto it = sequences_.find(sequence_id);
        if (it == sequences_.end())
            return;

        txn_deltas_[txn_id].removed_sequences.push_back(it->second);
        sequences_.erase(it);
    }

    types::MetaSequence*
    CatalogCache::get_sequence(const types::UUID& id)
    {
        const auto it = sequences_.find(id);
        return it == sequences_.end() ? nullptr : &it->second;
    }

    bool
    CatalogCache::exists_schema(const std::string& name)
    {
        return get_schema(name) != nullptr;
    }

    types::MetaSchema
    *
    CatalogCache::save_schema(const types::MetaSchema& ms, const types::UUID& txn_id)
    {
        auto it = schemas_.find(ms.id);
        if (it == schemas_.end())
            txn_deltas_[txn_id].added_schemas.push_back(ms.id);

        schemas_[ms.id] = ms;
        return &schemas_[ms.id];
    }

    std::vector<types::MetaTable*>
    CatalogCache::get_all_tables()
    {
        std::vector<types::MetaTable*> tables;
        tables.reserve(tables_.size());

        for (auto& table : tables_ | std::views::values)
            tables.push_back(&table);

        return tables;
    }

    std::vector<types::MetaTable*>
    CatalogCache::get_all_tables(const types::SchemaId& schema_id)
    {
        std::vector<types::MetaTable*> tables;
        tables.reserve(tables_.size());

        for (auto& table : tables_ | std::views::values)
            if (table.schema_id == schema_id)
                tables.push_back(&table);

        return tables;
    }

    std::vector<types::MetaSchema*>
    CatalogCache::get_all_schemas()
    {
        std::vector<types::MetaSchema*> schemas;
        schemas.reserve(schemas_.size());

        for (auto& schema : schemas_ | std::views::values)
            schemas.push_back(&schema);

        return schemas;
    }
} // namespace storage
