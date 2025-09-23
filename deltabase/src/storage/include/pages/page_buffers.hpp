#pragma once

#include "page.hpp"
#include "../cache/entity_cache.hpp"
#include "../objects/data_object.hpp"
#include "../objects/meta_object.hpp"

namespace storage {
    class PageBuffers {
        EntityCache<std::string, DataPage, DataPageAccessor> pages_;
        EntityCache<std::string, MetaSchema, MetaSchemaAccessor>& schemas_;
        EntityCache<std::string, MetaTable, MetaTableAccessor>& tables_;

        DataPage&
        create_page();

        bool
        has_available_page(uint64_t payload_size) noexcept;
        DataPage&
        get_available_page(uint64_t payload_size);

    public:
        PageBuffers(
            EntityCache<std::string, DataPage, DataPageAccessor>& pages,
            EntityCache<std::string, MetaSchema, MetaSchemaAccessor>& schemas,
            EntityCache<std::string, MetaTable, MetaTableAccessor>& tables
        );

        int
        insert_row(MetaTable& table, DataRow& row);
    };
}