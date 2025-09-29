#pragma once

#include "cache/accessors.hpp"
#include "page.hpp"
#include "../cache/entity_cache.hpp"
#include "../cache/key_extractor.hpp"
#include "../objects/data_object.hpp"
#include "../objects/meta_object.hpp"

namespace storage {
    class page_buffers {
        entity_cache<std::string, data_page, data_page_accessor, make_key> pages_;
        entity_cache<std::string, meta_schema, meta_schema_accessor, make_key>& schemas_;
        entity_cache<std::string, meta_table, meta_table_accessor, make_key>& tables_;

        data_page&
        create_page();

        bool
        has_available_page(uint64_t payload_size) noexcept;
        data_page&
        get_available_page(uint64_t payload_size);

    public:
        page_buffers(
            entity_cache<std::string, meta_schema, meta_schema_accessor, make_key>& schemas,
            entity_cache<std::string, meta_table, meta_table_accessor, make_key>& tables
        );

        int
        insert_row(meta_table& table, data_row& row);
    };
}