//
// Created by poproshaikin on 15.01.26.
//

#include "detached_file_io_manager.hpp"

#include "path.hpp"
#include "../../misc/include/exceptions.hpp"

namespace storage
{
    using DbGuard = std::lock_guard<DatabaseIoLockService::Mutex>;

    [[noreturn]] static void unsupported()
    {
        throw EngineException("no database attached", EngineException::Code::DB_NOT_ATTACHED);
    }

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
    DetachedFileIOManager::read_tables_meta() { unsupported(); }

    std::vector<types::MetaSchema>
    DetachedFileIOManager::read_schemas_meta() { unsupported(); }

    types::MetaSchema
    DetachedFileIOManager::read_schema_meta(const std::string&) { unsupported(); }

    types::MetaSchema
    DetachedFileIOManager::read_schema_meta(const types::UUID&) { unsupported(); }

    bool
    DetachedFileIOManager::exists_table(const std::string&, const std::string&) { unsupported(); }

    types::MetaTable
    DetachedFileIOManager::read_table_meta(const std::string&, const std::string&) { unsupported(); }

    types::MetaTable
    DetachedFileIOManager::read_table_meta(const types::UUID&) { unsupported(); }

    std::vector<types::DataPage>
    DetachedFileIOManager::read_table_data(const std::string&, const std::string&) { unsupported(); }

    std::vector<std::pair<types::UUID, std::vector<types::DataPage>>>
    DetachedFileIOManager::read_tables_data() { unsupported(); }

    std::unique_ptr<types::DataPage>
    DetachedFileIOManager::read_data_page(types::DataPageId) { unsupported(); }

    uint64_t
    DetachedFileIOManager::estimate_size(const types::DataRow&) { unsupported(); }

    void
    DetachedFileIOManager::write(const types::DataPage&, bool) { unsupported(); }

    void
    DetachedFileIOManager::write_mt(const types::MetaTable&, const std::string&, bool) { unsupported(); }

    void
    DetachedFileIOManager::write_mt(const types::MetaTable&, bool) { unsupported(); }

    void
    DetachedFileIOManager::write_cfg(const types::Config&) { unsupported(); }

    void
    DetachedFileIOManager::write_ms(const types::MetaSchema&, bool) { unsupported(); }

    void
    DetachedFileIOManager::delete_mt(const types::MetaTable&) { unsupported(); }

    void
    DetachedFileIOManager::delete_ms(const types::MetaSchema&) { unsupported(); }

    types::DataPage
    DetachedFileIOManager::create_page(const types::MetaTable&) { unsupported(); }

    types::DataPage
    DetachedFileIOManager::create_page(const types::MetaTable&, const types::DataPageId&) { unsupported(); }

    bool
    DetachedFileIOManager::exists_schema(const std::string&) { unsupported(); }

    std::unordered_map<types::TableId, std::vector<types::DataPageId>>
    DetachedFileIOManager::map_data_pages_for_table() { unsupported(); }

    std::unordered_map<types::TableId, std::vector<types::IndexId>>
    DetachedFileIOManager::map_index_files_for_table() { unsupported(); }

    types::IndexFile
    DetachedFileIOManager::create_index_file(
        const std::string&, const std::string&, const types::MetaIndex&
    ) { unsupported(); }

    std::unique_ptr<types::IndexFile>
    DetachedFileIOManager::read_index_file(const types::IndexId&) { unsupported(); }

    void
    DetachedFileIOManager::write(const types::IndexFile&, bool) { unsupported(); }

    types::MetaSequence
    DetachedFileIOManager::read_seq(const std::string&, const std::string&) { unsupported(); }

    void
    DetachedFileIOManager::write_seq(const types::MetaSequence&, bool) { unsupported(); }

    void
    DetachedFileIOManager::delete_seq(const types::MetaSequence&) { unsupported(); }

    std::vector<types::MetaSequence>
    DetachedFileIOManager::read_sequences() { unsupported(); }

} // namespace storage
