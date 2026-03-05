//
// Created by poproshaikin on 04.03.26.
//

#include "include/file_wal_io_manager.hpp"

#include "include/wal_serializer_factory.hpp"
#include "path.hpp"

namespace wal
{
    FileWalIOManager::FileWalIOManager(
        const fs::path& db_path,
        const std::string& db_name,
        types::Config::SerializerType serializer_type
    ) : db_path_(db_path), db_name_(db_name)
    {
        WalSerializerFactory serializer_factory;
        serializer_ = serializer_factory.make(serializer_type);

        auto path = storage::path_db_wal(db_path, db_name);
        if (fs::exists(path))
            fs::create_directory(path);
    }

    void
    FileWalIOManager::write_logs(const std::vector<types::WalRecord>& log) const
    {
        
    }

    void
    FileWalIOManager::append_log(const types::WalRecord& record)
    {
        dirty_.push_back(record);
    }

    void
    FileWalIOManager::flush()
    {
        if (dirty_.size() == 0)
            return;

        for (const auto& record : dirty_)
        {
            auto serialized = serializer_->serialize(record);

        }
    }
} // namespace wal
