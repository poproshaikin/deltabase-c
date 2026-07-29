//
// Created by poproshaikin on 3/28/26.
//

#ifndef DELTABASE_CATALOG_HPP
#define DELTABASE_CATALOG_HPP

#include "io_manager.hpp"
#include "../../types/include/meta_sequence.hpp"

#include <unordered_map>

namespace storage
{
    struct CatalogDelta
    {
        std::vector<types::UUID>         added_tables;
        std::vector<types::UUID>         added_schemas;
        std::vector<types::UUID>         added_sequences;

        std::vector<types::MetaTable>    removed_tables;
        std::vector<types::MetaSchema>   removed_schemas;
        std::vector<types::MetaSequence> removed_sequences;

        // before-images для UPDATE (ALTER TABLE и т.п.)
        std::vector<types::MetaTable>    updated_tables_before;
    };

    class CatalogCache
    {
        IIOManager& io_;
        std::unordered_map<types::UUID, types::MetaTable> tables_;
        std::unordered_map<types::UUID, types::MetaSchema> schemas_;
        std::unordered_map<types::UUID, types::MetaSequence> sequences_;

        std::unordered_map<types::UUID, CatalogDelta> txn_deltas_;

        void put(types::MetaTable table);
        void put(types::MetaSchema schema);
        void put(types::MetaSequence sequence);
        void put_or_update(types::MetaTable table, const types::UUID& txn_id);
        void delete_table(const types::UUID& table_id);

    public:
        explicit CatalogCache(IIOManager& io);
        ~CatalogCache();

        void
        hydrate();

        void
        flush();

        void
        put(types::MetaTable table, const types::UUID& txn_id);
        void
        put(types::MetaSchema schema, const types::UUID& txn_id);
        void
        put(types::MetaSequence sequence, const types::UUID& txn_id);

        void
        commit_txn(const types::UUID& txn_id);
        void
        rollback_txn(const types::UUID& txn_id);

        types::MetaTable*
        get_table(const types::UUID& id);
        types::MetaTable*
        get_table(const std::string& name, const types::UUID& schema_id);

        types::MetaTable*
        save_table(types::MetaTable&& mt, const types::UUID& txn_id);
        void
        delete_table(const types::UUID& table_id, const types::UUID& txn_id);

        types::MetaSchema*
        get_schema(const types::UUID& id);
        types::MetaSchema*
        get_schema(const std::string& name);
        types::MetaSchema
        *
        save_schema(const types::MetaSchema& ms, const types::UUID& txn_id);
        void
        delete_schema(const types::UUID& schema_id, const types::UUID& txn_id);

        types::MetaSequence*
        get_sequence(const types::UUID& id);
        void
        delete_sequence(const types::UUID& sequence_id, const types::UUID& txn_id);

        bool
        exists_schema(const std::string& name);

        std::vector<types::MetaTable*>
        get_all_tables();

        std::vector<types::MetaSchema*>
        get_all_schemas();
    };
} // namespace storage

#endif // DELTABASE_CATALOG_HPP
