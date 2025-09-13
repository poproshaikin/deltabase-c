#pragma once

#include "page.hpp"
#include "../cache/entity_cache.hpp"
#include "../objects/data_object.hpp"
#include "../objects/meta_object.hpp"

namespace storage {
    class PageBuffers {
        EntityCache<std::string, DataPage> pages_;
        EntityCache<std::string, MetaSchema>& schemas_;
        EntityCache<std::string, MetaTable>& tables_;

        DataPage&
        create_page();

        bool
        has_available_page(uint64_t payload_size) noexcept;
        DataPage&
        get_available_page(uint64_t payload_size);

    public:
        PageBuffers(
            EntityCache<std::string, MetaSchema>& schemas,
            EntityCache<std::string, MetaTable>& tables
        );

        int
        insert_row(MetaTable& table, DataRow& row);
    };
}