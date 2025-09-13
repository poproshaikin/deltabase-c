#pragma once 

#include <string>
#include <vector>
#include <optional>

#include "objects/meta_object.hpp"
#include "objects/data_object.hpp"

#include "cache/entity_cache.hpp"
#include "pages/page_buffers.hpp"
#include "wal/wal_manager.hpp"

namespace storage {
    class Storage {
        std::optional<std::string> db_name_;

        WalManager wal_;

        EntityCache<std::string, MetaSchema> schemas_;
        EntityCache<std::string, MetaTable> tables_;
        PageBuffers page_buffers_;

        void
        ensure_fs_initialize();
    public:
        /* 
            все прямые функции будут удалены
            оставляться умные методы,
            сами шедулируют планы,
            пропускаются через WAL,
            кешируются
            MetaRegistry станет подсистемой хранилища,
            все обращения к meta registry будут проходить через storage,
            meta registry будет упрощен, за кеширование будут отвечать специализированные шаблонные классы
            
        */
        Storage();

        // ----- Databases -----
        void
        create_database(const std::string& db_name);

        void
        drop_database(const std::string& db_name);

        bool
        exists_database(const std::string& db_name);

        std::vector<std::string>
        get_databases();

        // ----- Schemas -----

        void
        create_schema(const MetaSchema& schema);

        MetaSchema& 
        get_schema(const std::string& schema_name);
        MetaSchema& 
        get_schema(const std::string& schema_name, const std::string& db_name);
        MetaSchema&
        get_schema(const sql::TableIdentifier& identifier);
        MetaSchema&
        get_schema_by_id(const std::string& id);
        
        int
        drop_schema(const std::string& schema_name);                             
        int         
        drop_schema(const std::string& schema_name, const std::string& db_name);

        // ----- Tables -----

        void
        create_table(const MetaTable& table);

        bool
        exists_table(const std::string& name);
        bool
        exists_table(const std::string& name, const std::string& db_name);
        bool
        exists_table(const sql::TableIdentifier& identifier);
        bool
        exists_virtual_table(const sql::TableIdentifier& identifier);

        MetaTable&
        get_table(const std::string& name);
        MetaTable&
        get_table(const std::string& name, const std::string& db_name);
        MetaTable&
        get_table(const sql::TableIdentifier& identifier);
        MetaTable&
        get_table_by_id(const std::string& id) const;
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