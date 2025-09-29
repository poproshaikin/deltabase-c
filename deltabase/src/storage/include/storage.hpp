#pragma once 

#include <string>
#include <vector>
#include <optional>

#include "cache/key_extractor.hpp"
#include "objects/meta_object.hpp"
#include "objects/data_object.hpp"

#include "cache/entity_cache.hpp"
#include "pages/page_buffers.hpp"
#include "wal/wal_manager.hpp"

namespace storage {
    class storage {
        std::optional<std::string> db_name_;

        wal_manager wal_;
        file_manager fm_;

        std::optional<
            entity_cache<std::string, meta_schema, meta_schema_accessor, make_key>>
            schemas_;

        std::optional<entity_cache<std::string, meta_table, meta_table_accessor, make_key>>
            tables_;

        std::optional<page_buffers> page_buffers_;

        // Configuration

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

        storage(const fs::path& data_dir);
        storage(const fs::path& data_dir, const std::string& db_name);

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
        create_schema(meta_schema&& schema);

        bool
        exists_schema(const std::string& schema_name) const;
        bool
        exists_schema_by_id(const std::string& schema_id) const;

        meta_schema& 
        get_schema(const std::string& schema_name) const;
        meta_schema&
        get_schema(const sql::TableIdentifier& identifier) const; 
        meta_schema&
        get_schema_by_id(const std::string& id) const;
        
        void
        drop_schema(const std::string& schema_name);                             

        // ----- Tables -----

        void
        create_table(meta_table&& table);

        bool
        exists_table(const std::string& schema_name, const std::string& name);
        bool
        exists_table(const sql::TableIdentifier& identifier);
        bool
        exists_table_by_id(const std::string& table_id);
        bool
        exists_virtual_table(const sql::TableIdentifier& identifier);

        meta_table&
        get_table(const std::string& name);
        meta_table&
        get_table(const sql::TableIdentifier& identifier);
        meta_table&
        get_table_by_id(const std::string& id) ;
        meta_table&&
        get_virtual_meta_table(const sql::TableIdentifier& identifier);
        data_table
        get_virtual_data_table(const sql::TableIdentifier& identifier);

        void
        update_table(const meta_table& new_table);

        // ----- Data -----       

        void
        insert_row(meta_table& table, data_row row);

        uint64_t
        update_rows_by_filter(
            meta_table& table, const data_filter& filter, const data_row_update& update
        );

        uint64_t
        delete_rows_by_filter(meta_table& table, const std::optional<data_filter>& filter);

        data_table
        seq_scan(
            const meta_table& table,
            const std::vector<std::string>& column_names,
            const std::optional<data_filter>& filter
        );
    };
}