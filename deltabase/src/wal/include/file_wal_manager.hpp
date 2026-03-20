//
// Created by poproshaikin on 06.03.26.
//

#ifndef DELTABASE_FILE_WAL_MANAGER_HPP
#define DELTABASE_FILE_WAL_MANAGER_HPP

#include "config.hpp"
#include "wal_manager.hpp"
#include "wal_serializer.hpp"

#include <filesystem>
#include <fstream>
#include <memory>

namespace wal
{
    namespace fs = std::filesystem;

    class FileWalManager : public IWalManager
    {
        fs::path db_path_;
        std::string db_name_;
        types::Lsn next_lsn_;

        std::unique_ptr<IWalSerializer> serializer_;

        // Cache for performance: flushed records already on disk, dirty records in memory
        std::vector<types::WalRecord> flushed_;
        std::vector<types::WalRecord> dirty_;

        static constexpr uint64_t MAX_RECORDS_PER_LOGFILE = 1000;

        // Helper methods
        void
        write_logs(const std::vector<types::WalRecord>& logs);

        std::vector<types::WalRecord>
        read_logs_from_file(const fs::path& file_path);

        // Load all existing WAL records from disk into flushed_ cache
        void
        hydrate_cache();

    public:
        FileWalManager(
            const fs::path& db_path,
            const std::string& db_name,
            types::Config::SerializerType serializer_type
        );

        // IWalManager interface
        void
        append_log(const types::WalRecord& record) override;

        void
        append_log(const std::vector<types::WalRecord>& records) override;

        types::WalRecord
        read_log(types::Lsn lsn) override;

        void
        flush() override;

        void
        sync() override;

        std::vector<types::WalRecord>
        read_all_logs() override;

        types::Lsn
        get_next_lsn() const override;
    };
} // namespace wal

#endif // DELTABASE_FILE_WAL_MANAGER_HPP
