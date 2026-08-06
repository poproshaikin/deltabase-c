//
// Created by poproshaikin on 3/28/26.
//

#ifndef DELTABASE_CATALOG_HPP
#define DELTABASE_CATALOG_HPP

#include "io_manager.hpp"
#include "../../types/include/meta_sequence.hpp"

#include <mutex>
#include <unordered_map>

namespace storage
{
    template <typename TValue>
    struct CatalogEntry
    {
        TValue value;
        bool dirty = false;
        types::LSN last_lsn = 0;

        CatalogEntry(TValue&& value) : value(std::move(value))
        {
        }
    };

    class CatalogCache
    {

        IIOManager& io_;
        mutable std::mutex mutex_;
        std::unordered_map<types::UUID, CatalogEntry<types::MetaTable>> tables_;
        std::unordered_map<types::UUID, CatalogEntry<types::MetaSchema>> schemas_;
        std::unordered_map<types::UUID, CatalogEntry<types::MetaSequence>> sequences_;

    public:
        explicit CatalogCache(IIOManager& io);
        ~CatalogCache();

        void
        hydrate();

        void
        flush();
        void
        flush(types::LSN max_lsn);

        void
        put(types::MetaTable table, types::LSN last_lsn);
        void
        put(types::MetaSchema schema, types::LSN last_lsn);
        void
        put(types::MetaSequence sequence, types::LSN last_lsn);

        void
        mark_dirty(const types::MetaTable* table, types::LSN last_lsn);
        void
        mark_dirty(const types::MetaSchema* schema, types::LSN last_lsn);
        void
        mark_dirty(const types::MetaSequence* sequence, types::LSN last_lsn);

        types::MetaTable*
        get_table(const types::UUID& id);
        const types::MetaTable*
        get_table(const types::UUID& id) const;
        types::MetaTable*
        get_table(const std::string& name, const types::UUID& schema_id);

        types::MetaTable*
        save_table(types::MetaTable&& mt, types::LSN last_lsn);
        void
        delete_table(const types::UUID& table_id);

        types::MetaSchema*
        get_schema(const types::UUID& id);
        types::MetaSchema*
        get_schema(const std::string& name);
        types::MetaSchema*
        save_schema(const types::MetaSchema& ms, types::LSN last_lsn);
        void
        delete_schema(const types::UUID& schema_id);

        types::MetaSequence*
        get_sequence(const types::UUID& id);
        void
        delete_sequence(const types::UUID& sequence_id);

        bool
        exists_schema(const std::string& name);

        std::vector<types::MetaTable*>
        get_all_tables();

        std::vector<types::MetaTable*>
        get_all_tables(const types::SchemaId& schema_id);

        std::vector<types::MetaSchema*>
        get_all_schemas();

    private:
        // All *_impl methods assume the caller already holds whatever lock guards
        // tables_/schemas_/sequences_. They must only call other *_impl methods
        // internally, never the public (locking) API above -- calling a locking
        // public method from here would re-enter the same (non-recursive) lock on
        // the same thread and deadlock.

        void
        hydrate_impl();

        void
        flush_impl();
        void
        flush_impl(types::LSN max_lsn);

        void
        put_impl(types::MetaTable table, types::LSN last_lsn);
        void
        put_impl(types::MetaSchema schema, types::LSN last_lsn);
        void
        put_impl(types::MetaSequence sequence, types::LSN last_lsn);

        void
        mark_dirty_impl(const types::MetaTable* table, types::LSN last_lsn);
        void
        mark_dirty_impl(const types::MetaSchema* schema, types::LSN last_lsn);
        void
        mark_dirty_impl(const types::MetaSequence* sequence, types::LSN last_lsn);

        types::MetaTable*
        get_table_impl(const types::UUID& id);
        const types::MetaTable*
        get_table_impl(const types::UUID& id) const;
        types::MetaTable*
        get_table_impl(const std::string& name, const types::UUID& schema_id);

        types::MetaTable*
        save_table_impl(types::MetaTable&& mt, types::LSN last_lsn);
        void
        delete_table_impl(const types::UUID& table_id);

        types::MetaSchema*
        get_schema_impl(const types::UUID& id);
        types::MetaSchema*
        get_schema_impl(const std::string& name);
        types::MetaSchema*
        save_schema_impl(const types::MetaSchema& ms, types::LSN last_lsn);
        void
        delete_schema_impl(const types::UUID& schema_id);

        types::MetaSequence*
        get_sequence_impl(const types::UUID& id);
        void
        delete_sequence_impl(const types::UUID& sequence_id);

        bool
        exists_schema_impl(const std::string& name);

        std::vector<types::MetaTable*>
        get_all_tables_impl();

        std::vector<types::MetaTable*>
        get_all_tables_impl(const types::SchemaId& schema_id);

        std::vector<types::MetaSchema*>
        get_all_schemas_impl();
    };
} // namespace storage

#endif // DELTABASE_CATALOG_HPP
