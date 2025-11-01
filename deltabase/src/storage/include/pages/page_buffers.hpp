#pragma once

#include "../cache/accessors.hpp"
#include "page.hpp"
#include "../cache/entity_cache.hpp"
#include "../cache/key_extractor.hpp"
#include "../objects/data_object.hpp"
#include "../objects/meta_object.hpp"

namespace storage {
    class PageBuffers {
        std::string db_name_;
        FileManager& fm_;
        EntityCache<std::string, DataPage, DataPageAccessor, make_key> pages_;
        EntityCache<std::string, MetaSchema, MetaSchemaAccessor, make_key>& schemas_;
        EntityCache<std::string, MetaTable, MetaTableAccessor, make_key>& tables_;

        void
        load();

        DataPage&
        create_page();
        void
        update_page(DataPage& page);

        bool
        has_available_page(uint64_t payload_size) noexcept;
        DataPage&
        get_available_page(uint64_t payload_size);

    public:
        PageBuffers(
            const std::string& db_name,
            FileManager& fm,
            EntityCache<std::string, MetaSchema, MetaSchemaAccessor, make_key>& schemas,
            EntityCache<std::string, MetaTable, MetaTableAccessor, make_key>& tables
        );

        int
        insert_row(MetaTable& table, DataRow& row);

        void
        flush();
    };
}