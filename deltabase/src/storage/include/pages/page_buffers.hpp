#pragma once

#include "../cache/accessors.hpp"
#include "page.hpp"
#include "../cache/entity_cache.hpp"
#include "../cache/key_extractor.hpp"
#include "../objects/data_object.hpp"
#include "../objects/meta_object.hpp"
#include "cache/registry.hpp"

namespace storage
{
    class PageBuffers
    {
        std::string db_name_;
        FileManager& fm_;
        EntityCache<std::string, DataPage, DataPageAccessor, make_key> pages_;
        Registry<std::string, MetaSchema, make_key>& schemas_;
        Registry<std::string, MetaTable, make_key>& tables_;

        void
        load();

        DataPage&
        create_page(const MetaTable& table);
        void
        update_page(DataPage& page);

        bool
        has_available_page(uint64_t payload_size) noexcept;
        DataPage&
        get_available_page(uint64_t payload_size);

        std::vector<std::reference_wrapper<DataRow>>
        scan_with_filter(const MetaTable& table, const DataFilter& filter);

    public:
        PageBuffers(
            const std::string& db_name,
            FileManager& fm,
            Registry<std::string, MetaSchema, make_key>& schemas,
            Registry<std::string, MetaTable, make_key>& tables
        );

        int
        insert_row(MetaTable& table, DataRow& row);

        void
        flush();

        uint64_t
        update_rows(const MetaTable& table, const DataFilter& filter, const DataRowUpdate& update);
    };
} // namespace storage