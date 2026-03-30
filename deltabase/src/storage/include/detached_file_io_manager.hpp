//
// Created by poproshaikin on 15.01.26.
//

#ifndef DELTABASE_DETACHED_FILE_IO_MANAGER_HPP
#define DELTABASE_DETACHED_FILE_IO_MANAGER_HPP
#include "binary_serializer.hpp"
#include "io_manager.hpp"

namespace storage
{
    namespace fs = std::filesystem;

    class DetachedFileIOManager final : public IIOManager
    {
        fs::path db_path_;
        std::unique_ptr<IBinarySerializer> serializer_;
    public:
        explicit
        DetachedFileIOManager(const fs::path& db_path);

        void
        init() override;

        bool
        exists_db(const std::string& name) override;

        // Unsupported methods

        std::vector<types::MetaTable>
        read_tables_meta() override;

        std::vector<types::MetaSchema>
        read_schemas_meta() override;

        types::MetaSchema
        read_schema_meta(const std::string& schema_name) override;

        bool
        exists_table(const std::string& table_name, const std::string& schema_name) override;

        types::MetaTable
        read_table_meta(const std::string& table_name, const std::string& schema_name) override;

        std::vector<types::DataPage>
        read_table_data(const std::string& table_name, const std::string& schema_name) override;

        std::vector<std::pair<types::Uuid, std::vector<types::DataPage>>>
        read_tables_data() override;

        uint64_t
        estimate_size(const types::DataRow& row) override;

        void
        write_page(const types::DataPage& page, bool fsync) override;

        void
        write_mt(const types::MetaTable& table, const std::string& schema_name, bool fsync) override;

        void
        write_mt(const types::MetaTable& table, bool fsync) override;

        void
        write_cfg(const types::Config& cfg) override;

        void
        write_ms(const types::MetaSchema& ms, bool fsync) override;

        void
        delete_mt(const types::MetaTable& table) override;

        void
        delete_ms(const types::MetaSchema& schema) override;

        types::MetaSchema
        read_schema_meta(const types::Uuid& schema_id) override;

        types::DataPage
        create_page(const types::MetaTable& mt) override;

        types::DataPage
        create_page(const types::MetaTable& mt, const types::PageId& page_id) override;

        bool
        exists_schema(const std::string& schema_name) override;

        types::MetaTable
        read_table_meta(const types::Uuid& table_id) override;

        std::unique_ptr<types::DataPage>
        read_data_page(types::PageId id) override;

        std::unordered_map<types::TableId, std::vector<types::PageId>>
        map_tables_pages() override;
    };
} // namespace storage

#endif //DELTABASE_DETACHED_FILE_IO_MANAGER_HPP