//
// Created by poproshaikin on 15.01.26.
//

#ifndef DELTABASE_DETACHED_FILE_IO_MANAGER_HPP
#define DELTABASE_DETACHED_FILE_IO_MANAGER_HPP
#include "db_io_lock_service.hpp"
#include "io_manager.hpp"
#include "storage_serializer.hpp"

#include <memory>

namespace storage
{
    namespace fs = std::filesystem;

    class DetachedFileIOManager final : public IIOManager
    {
        fs::path db_path_;
        std::unique_ptr<IStorageSerializer> serializer_;
        std::shared_ptr<DatabaseIoLockService> io_lock_service_;
        std::shared_ptr<DatabaseIoLockService::Mutex> db_mutex_;

    public:
        explicit DetachedFileIOManager(const fs::path& db_path);

        DetachedFileIOManager(
            const fs::path& db_path,
            std::shared_ptr<DatabaseIoLockService> io_lock_service
        );

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
        write_mt(
            const types::MetaTable& table, const std::string& schema_name, bool fsync
        ) override;

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
        create_page(const types::MetaTable& mt, const types::DataPageId& page_id) override;

        bool
        exists_schema(const std::string& schema_name) override;

        types::MetaTable
        read_table_meta(const types::Uuid& table_id) override;

        std::unique_ptr<types::DataPage>
        read_data_page(types::DataPageId id) override;

        std::unordered_map<types::TableId, std::vector<types::DataPageId>>
        map_data_pages_for_table() override;

        std::unordered_map<types::TableId, std::vector<types::IndexId>>
        map_index_files_for_table() override;

        types::IndexFile
        create_index_file(
            const std::string& string, const std::string& table_name, const types::MetaIndex& mi
        ) override;

        std::unique_ptr<types::IndexFile>
        read_index_file(const types::IndexId& index_id) override;

        void
        write_index_file(const types::IndexFile& index_file, bool fsync) override;
    };
} // namespace storage

#endif // DELTABASE_DETACHED_FILE_IO_MANAGER_HPP