//
// Created by poproshaikin on 15.01.26.
//

#include "detached_file_io_manager.hpp"

#include "detached_db_instance.hpp"
#include "path.hpp"

namespace storage
{
    using DbGuard = std::lock_guard<DatabaseIoLockService::Mutex>;

    DetachedFileIOManager::DetachedFileIOManager(const fs::path& db_path)
        : DetachedFileIOManager(db_path, DatabaseIoLockService::shared())
    {
    }

    DetachedFileIOManager::DetachedFileIOManager(
        const fs::path& db_path, std::shared_ptr<DatabaseIoLockService> io_lock_service
    )
        : db_path_(db_path), io_lock_service_(std::move(io_lock_service))
    {
        if (!io_lock_service_)
            io_lock_service_ = DatabaseIoLockService::shared();

        db_mutex_ = io_lock_service_->mutex_for(db_path_, "detached");
    }

    void
    DetachedFileIOManager::init()
    {
    }

    bool
    DetachedFileIOManager::exists_db(const std::string& name)
    {
        DbGuard guard(*db_mutex_);
        auto path = path_db(db_path_, name);
        return fs::exists(path) && fs::is_directory(path);
    }

    std::vector<types::MetaTable>
    DetachedFileIOManager::read_tables_meta()
    {
        throw std::logic_error("DetachedFileIOManager::load_tables_meta: unsupported method");
    }

    std::vector<types::MetaSchema>
    DetachedFileIOManager::read_schemas_meta()
    {
        throw std::logic_error("DetachedFileIOManager::load_schemas_meta: unsupported method");
    }

    types::MetaSchema
    DetachedFileIOManager::read_schema_meta(const std::string& schema_name)
    {
        throw std::logic_error("DetachedFileIOManager::load_schema_meta: unsupported method");
    }

    bool
    DetachedFileIOManager::exists_table(
        const std::string& table_name, const std::string& schema_name
    )
    {
        throw std::logic_error("DetachedFileIOManager::exists_table: unsupported method");
    }

    types::MetaTable
    DetachedFileIOManager::read_table_meta(
        const std::string& table_name, const std::string& schema_name
    )
    {
        throw std::logic_error("DetachedFileIOManager::load_table_meta: unsupported method");
    }
    std::vector<types::DataPage>
    DetachedFileIOManager::read_table_data(
        const std::string& table_name, const std::string& schema_name
    )
    {
        throw std::logic_error("DetachedFileIOManager::load_table_data: unsupported method");
    }

    std::vector<std::pair<types::UUID, std::vector<types::DataPage>>>
    DetachedFileIOManager::read_tables_data()
    {
        throw std::logic_error("DetachedFileIOManager::load_tables_data: unsupported method");
    }

    uint64_t
    DetachedFileIOManager::estimate_size(const types::DataRow& row)
    {
        throw std::logic_error("DetachedFileIOManager::estimate_size: unsupported method");
    }

    void
    DetachedFileIOManager::write_page(const types::DataPage& page, bool fsync)
    {
        throw std::logic_error("DetachedFileIOManager::write_page: unsupported method");
    }

    void
    DetachedFileIOManager::write_mt(const types::MetaTable& table, const std::string& schema_name, bool fsync)
    {
        throw std::logic_error("DetachedFileIOManager::write_mt: unsupported method");
    }

    void
    DetachedFileIOManager::write_mt(const types::MetaTable& table, bool fsync)
    {
        throw std::logic_error("DetachedFileIOManager::write_mt: unsupported method");
    }

    void
    DetachedFileIOManager::write_cfg(const types::Config& cfg)
    {
        throw std::logic_error("DetachedFileIOManager::write_cfg: unsupported method");
    }

    void
    DetachedFileIOManager::write_ms(const types::MetaSchema& ms, bool fsync)
    {
        throw std::logic_error("DetachedFileIOManager::write_ms: unsupported method");
    }

    void
    DetachedFileIOManager::delete_mt(const types::MetaTable& table)
    {
        throw std::logic_error("DetachedFileIOManager::delete_mt: unsupported method");
    }

    void
    DetachedFileIOManager::delete_ms(const types::MetaSchema& schema)
    {
        throw std::logic_error("DetachedFileIOManager::delete_ms: unsupported method");
    }

    types::MetaSchema
    DetachedFileIOManager::read_schema_meta(const types::UUID& schema_id)
    {
        throw std::logic_error("DetachedFileIOManager::load_schema_meta: unsupported method");
    }

    types::DataPage
    DetachedFileIOManager::create_page(const types::MetaTable& mt)
    {
        throw std::logic_error("DetachedFileIOManager::create_page: unsupported method");
    }

    types::DataPage
    DetachedFileIOManager::create_page(const types::MetaTable& mt, const types::DataPageId& page_id)
    {
        throw std::logic_error("DetachedFileIOManager::create_page: unsupported method");
    }

    bool
    DetachedFileIOManager::exists_schema(const std::string& schema_name)
    {
        throw std::logic_error("DetachedFileIOManager::exists_schema: unsupported method");
    }

    types::MetaTable
    DetachedFileIOManager::read_table_meta(const types::UUID& table_id)
    {
        throw std::logic_error("DetachedFileIOManager::read_table_meta: unsupported method");
    }

    std::unique_ptr<types::DataPage>
    DetachedFileIOManager::read_data_page(types::DataPageId id)
    {
        throw std::logic_error("DetachedFileIOManager::read_data_page: unsupported method");
    }
    std::unordered_map<types::TableId, std::vector<types::DataPageId>>
    DetachedFileIOManager::map_data_pages_for_table()
    {
        throw std::logic_error("DetachedFileIOManager::map_tables_pages: unsupported method");
    }

    std::unordered_map<types::TableId, std::vector<types::IndexId>>
    DetachedFileIOManager::map_index_files_for_table()
    {
        throw std::logic_error("DetachedFileIOManager::map_index_files_for_table: unsupported method");
    }

    types::IndexFile
    DetachedFileIOManager::create_index_file(
        const std::string& string, const std::string& table_name, const types::MetaIndex& mi
    )
    {
        throw std::logic_error("DetachedFileIOManager::create_index_file: unsupported method");
    }

    std::unique_ptr<types::IndexFile>
    DetachedFileIOManager::read_index_file(const types::IndexId& index_id)
    {
        throw std::logic_error("DetachedFileIOManager::read_index_file: unsupported method");
    }

    void
    DetachedFileIOManager::write_index_file(const types::IndexFile& index_file, bool fsync)
    {
        throw std::logic_error("DetachedFileIOManager::write_index_file: unsupported method");
    }
} // namespace storage