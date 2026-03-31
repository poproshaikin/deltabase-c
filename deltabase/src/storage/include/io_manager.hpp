//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_IO_MANAGER_HPP
#define DELTABASE_IO_MANAGER_HPP
#include "../../types/include/config.hpp"
#include "../../types/include/data_page.hpp"
#include "../../types/include/meta_schema.hpp"
#include "../../types/include/meta_table.hpp"
#include "LRU_policy.hpp"
#include "cache.hpp"
#include "index_file.hpp"
#include <vector>

namespace storage
{
    class IIOManager
    {
    public:
        virtual ~IIOManager() = default;

        virtual void
        init() = 0;

        virtual std::vector<types::MetaTable>
        read_tables_meta() = 0;

        virtual std::vector<types::MetaSchema>
        read_schemas_meta() = 0;

        virtual types::MetaSchema
        read_schema_meta(const std::string& schema_name) = 0;

        virtual types::MetaSchema
        read_schema_meta(const types::Uuid& schema_id) = 0;

        virtual bool
        exists_table(const std::string& table_name, const std::string& schema_name) = 0;

        virtual types::MetaTable
        read_table_meta(const types::Uuid& table_id) = 0;

        virtual types::MetaTable
        read_table_meta(const std::string& table_name, const std::string& schema_name) = 0;

        virtual std::vector<types::DataPage>
        read_table_data(const std::string& table_name, const std::string& schema_name) = 0;

        virtual std::vector<std::pair<types::TableId, std::vector<types::DataPage> > >
        read_tables_data() = 0;

        virtual std::unique_ptr<types::DataPage>
        read_data_page(types::DataPageId id) = 0;

        virtual uint64_t
        estimate_size(const types::DataRow& row) = 0;

        virtual void
        write_page(const types::DataPage& page, bool fsync = false) = 0;

        virtual void
        write_mt(const types::MetaTable& table, const std::string& schema_name, bool fsync = false) = 0;

        virtual void
        write_mt(const types::MetaTable& table, bool fsync = false) = 0;

        virtual void
        write_cfg(const types::Config& cfg) = 0;

        virtual bool
        exists_db(const std::string& name) = 0;

        virtual void
        write_ms(const types::MetaSchema& ms, bool fsync = false) = 0;

        virtual void
        delete_mt(const types::MetaTable& table) = 0;

        virtual void
        delete_ms(const types::MetaSchema& schema) = 0;

        virtual types::DataPage
        create_page(const types::MetaTable& mt) = 0;

        virtual types::DataPage
        create_page(const types::MetaTable& mt, const types::DataPageId& page_id) = 0;

        virtual bool
        exists_schema(const std::string& schema_name) = 0;

        virtual std::unordered_map<types::TableId, std::vector<types::DataPageId>>
        map_data_pages_for_table() = 0;

        virtual std::unordered_map<types::TableId, std::vector<types::IndexId>>
        map_index_files_for_table() = 0;

        virtual void
        create_index_file(const std::string& string, const std::string& table_name, const types::MetaIndex& mi) = 0;

        virtual std::unique_ptr<types::IndexFile>
        read_index_file(const types::IndexId& index_id) = 0;

        virtual void
        write_index_file(const types::IndexFile& index_file, bool fsync = false) = 0;
    };
} // namespace storage

#endif //DELTABASE_IO_MANAGER_HPP