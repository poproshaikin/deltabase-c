#pragma once 

#include <string>
#include <vector>
#include <optional>

#include "cache/key_extractor.hpp"
#include "cache/entity_cache.hpp"
#include "objects/meta_object.hpp"
#include "objects/data_object.hpp"
#include "pages/page_buffers.hpp"
#include "wal/wal_manager.hpp"
#include "checkpoint.hpp"

namespace storage {
    class Storage {
        std::optional<std::string> db_name_;

        FileManager fm_;

        std::optional<EntityCache<std::string, MetaSchema, MetaSchemaAccessor, make_key>>
            schemas_;
        std::optional<EntityCache<std::string, MetaTable, MetaTableAccessor, make_key>> 
            tables_;

        std::optional<PageBuffers> page_buffers_;
        std::optional<CheckpointManager> checkpoint_ctl_;
        std::optional<WalManager> wal_;

        fs::path data_dir_;

        void
        ensure_fs_initialize();
        void
        ensure_attached(const std::string& method_name = "ensure_attached") const;

        std::optional<std::string>
        find_schema_key(const std::string& schema_id) const;

        std::optional<std::string>
        find_table_key(const std::string& table_id) const;
    public:

        Storage(const fs::path& data_dir);
        Storage(const fs::path& data_dir, const std::string& db_name);

        void
        attach_db(const std::string& db_name);

        // ----- Databases -----
        void
        create_database(const std::string& db_name);

        void
        drop_database(const std::string& db_name);

        bool
        exists_database(const std::string& db_name);

        // ----- Schemas -----

        void
        create_schema(MetaSchema&& schema);

        bool
        exists_schema(const std::string& schema_name) const;
        bool
        exists_schema_by_id(const std::string& schema_id) const;

        MetaSchema& 
        get_schema(const std::string& schema_name) const;
        MetaSchema&
        get_schema(const sql::TableIdentifier& identifier) const; 
        MetaSchema&
        get_schema_by_id(const std::string& id) const;
        
        void
        drop_schema(const std::string& schema_name);                             

        // ----- Tables -----

        void
        create_table(MetaTable&& table);

        bool
        exists_table(const std::string& schema_name, const std::string& name);
        bool
        exists_table(const sql::TableIdentifier& identifier);
        bool
        exists_table_by_id(const std::string& table_id);
        bool
        exists_virtual_table(const sql::TableIdentifier& identifier);

        MetaTable&
        get_table(const std::string& name);
        MetaTable&
        get_table(const sql::TableIdentifier& identifier);
        MetaTable&
        get_table_by_id(const std::string& id) ;
        MetaTable&&
        get_virtual_meta_table(const sql::TableIdentifier& identifier);
        DataTable
        get_virtual_data_table(const sql::TableIdentifier& identifier);

        void
        update_table(const MetaTable& new_table);

        // ----- Data -----       

        void
        insert_row(MetaTable& table, DataRow row);

        uint64_t
        update_rows_by_filter(
            MetaTable& table, const DataFilter& filter, const DataRowUpdate& update
        );

        uint64_t
        delete_rows_by_filter(MetaTable& table, const std::optional<DataFilter>& filter);

        DataTable
        seq_scan(
            const MetaTable& table,
            const std::vector<std::string>& column_names,
            const std::optional<DataFilter>& filter
        );
    };
}