//
// Created by poproshaikin on 06.03.26.
//

#ifndef DELTABASE_FILE_WAL_MANAGER_HPP
#define DELTABASE_FILE_WAL_MANAGER_HPP

#include "config.hpp"
#include "db_io_lock_service.hpp"
#include "wal_manager.hpp"
#include "wal_serializer.hpp"

#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>

namespace wal
{
    namespace fs = std::filesystem;

    class FileWalManager : public IWALManager
    {
        fs::path db_path_;
        std::string db_name_;
        types::LSN next_lsn_;
        types::LSN flushed_lsn_;
        std::shared_ptr<storage::DatabaseIoLockService> io_lock_service_;
        std::shared_ptr<storage::DatabaseIoLockService::Mutex> db_mutex_;

        mutable std::mutex mtx_;
        std::condition_variable cv_;
        bool flush_in_progress_ = false;

        std::unique_ptr<IWalSerializer> serializer_;

        // Cache for performance: flushed records already on disk, dirty records in memory
        std::vector<types::WALRecord> flushed_;
        std::vector<types::WALRecord> dirty_;

        static constexpr uint64_t MAX_RECORDS_PER_LOGFILE = 1000;

        // Helper methods
        void
        write_logs(const std::vector<types::WALRecord>& logs);

        std::vector<types::WALRecord>
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

        FileWalManager(
            const fs::path& db_path,
            const std::string& db_name,
            types::Config::SerializerType serializer_type,
            std::shared_ptr<storage::DatabaseIoLockService> io_lock_service
        );

        // IWalManager interface
        types::LSN
        append_log(const types::WALRecord& record) override;

        types::LSN
        append_log(const std::vector<types::WALRecord>& records) override;

        types::WALRecord
        read_log(types::LSN lsn) override;

        void
        wait_for_durable(types::LSN lsn) override;

        void
        commit_wait(types::LSN lsn);

        void
        flush() override;

        void
        sync() override;

        std::vector<types::WALRecord>
        read_all_logs() override;

        types::LSN
        get_next_lsn() const override;
    };
} // namespace wal

#endif // DELTABASE_FILE_WAL_MANAGER_HPP
