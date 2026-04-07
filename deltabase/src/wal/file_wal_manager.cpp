//
// Created by poproshaikin on 06.03.26.
//

#include "include/file_wal_manager.hpp"

#include "include/wal_serializer_factory.hpp"
#include "path.hpp"

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <stdexcept>
#include <string>

namespace wal
{
    using namespace types;
    using namespace misc;
    using DbGuard = std::lock_guard<storage::DatabaseIoLockService::Mutex>;

    FileWalManager::FileWalManager(
        const fs::path& db_path,
        const std::string& db_name,
        Config::SerializerType serializer_type
    )
        : FileWalManager(db_path, db_name, serializer_type, storage::DatabaseIoLockService::shared())
    {
    }

    FileWalManager::FileWalManager(
        const fs::path& db_path,
        const std::string& db_name,
        Config::SerializerType serializer_type,
        std::shared_ptr<storage::DatabaseIoLockService> io_lock_service
    )
        : db_path_(db_path),
          db_name_(db_name),
          next_lsn_(1),
          flushed_lsn_(0),
          io_lock_service_(std::move(io_lock_service))
    {
        if (!io_lock_service_)
            io_lock_service_ = storage::DatabaseIoLockService::shared();

        WalSerializerFactory factory;
        serializer_ = factory.make(serializer_type);
        db_mutex_ = io_lock_service_->mutex_for(db_path_, db_name_);

        // Create WAL directory if it doesn't exist
        auto wal_dir = storage::path_db_wal(db_path_, db_name_);
        if (!fs::exists(wal_dir))
        {
            fs::create_directories(wal_dir);
        }

        // Ensure a first WAL file exists for an empty WAL directory.
        if (fs::is_empty(wal_dir))
        {
            auto first_file_path =
                storage::path_db_wal_logfile(db_path_, db_name_, 1, MAX_RECORDS_PER_LOGFILE);

            int fd = open(first_file_path.c_str(), O_WRONLY | O_CREAT, 0644);
            if (fd < 0)
                throw std::runtime_error("Failed to create first WAL log file");

            close(fd);
        }

        // Hydrate cache: load all existing WAL records into memory
        hydrate_cache();
    }

    void
    FileWalManager::hydrate_cache()
    {
        auto dir = storage::path_db_wal(db_path_, db_name_);

        // If directory is empty, nothing to hydrate
        if (!fs::exists(dir) || fs::is_empty(dir))
            return;

        // Collect all log files and sort by first_lsn
        std::vector<std::pair<uint64_t, fs::path>> log_files;

        for (const auto& entry : fs::directory_iterator(dir))
        {
            if (!entry.is_regular_file())
                continue;

            auto filename = entry.path().filename().string();
            auto underscore_pos = filename.find('_');
            if (underscore_pos == std::string::npos)
                continue;

            uint64_t first_lsn = std::stoull(filename.substr(0, underscore_pos));
            log_files.emplace_back(first_lsn, entry.path());
        }

        // Sort by LSN to read in correct order
        std::sort(
            log_files.begin(),
            log_files.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; }
        );

        // Read each file and populate flushed_ cache
        for (const auto& [first_lsn, file_path] : log_files)
        {
            auto file_logs = read_logs_from_file(file_path);
            flushed_.insert(flushed_.end(), file_logs.begin(), file_logs.end());

            // Update next_lsn to be after the last loaded record
            for (const auto& record : file_logs)
            {
                auto record_lsn = std::visit([](const auto& rec) { return rec.lsn; }, record);
                if (record_lsn >= next_lsn_)
                    next_lsn_ = record_lsn + 1;
            }
        }

        // Trust record payload LSNs over filenames to keep correct logical order.
        std::sort(
            flushed_.begin(),
            flushed_.end(),
            [](const WALRecord& lhs, const WALRecord& rhs)
            {
                auto l_lsn = std::visit([](const auto& rec) { return rec.lsn; }, lhs);
                auto r_lsn = std::visit([](const auto& rec) { return rec.lsn; }, rhs);
                return l_lsn < r_lsn;
            }
        );

        if (!flushed_.empty())
        {
            flushed_lsn_ = std::visit([](const auto& rec) { return rec.lsn; }, flushed_.back());
        }
    }

    LSN
    FileWalManager::append_log(const WALRecord& record)
    {
        DbGuard guard(*db_mutex_);
        std::lock_guard lk(mtx_);
        auto lsn = next_lsn_++;

        auto record_with_lsn = std::visit(
            [lsn](auto rec) -> WALRecord
            {
                rec.lsn = lsn;
                return rec;
            },
            record
        );

        dirty_.push_back(record_with_lsn);
        return lsn;
    }

    LSN
    FileWalManager::append_log(const std::vector<WALRecord>& records)
    {
        DbGuard guard(*db_mutex_);
        LSN last_lsn = 0;
        for (const auto& record : records)
        {
            last_lsn = append_log(record);
        }

        return last_lsn;
    }

    WALRecord
    FileWalManager::read_log(LSN lsn)
    {
        DbGuard guard(*db_mutex_);
        std::lock_guard lk(mtx_);

        // Check dirty buffer first (most recent records)
        for (const auto& record : dirty_)
        {
            auto record_lsn = std::visit([](const auto& rec) { return rec.lsn; }, record);
            if (record_lsn == lsn)
                return record;
        }

        // Check flushed cache (hydrated from disk at startup)
        for (const auto& record : flushed_)
        {
            auto record_lsn = std::visit([](const auto& rec) { return rec.lsn; }, record);
            if (record_lsn == lsn)
                return record;
        }

        // Not found - record doesn't exist
        throw std::runtime_error("WAL record not found: LSN " + std::to_string(lsn));
    }

    void
    FileWalManager::wait_for_durable(LSN lsn)
    {
        DbGuard guard(*db_mutex_);
        std::unique_lock lk(mtx_);
        while (flushed_lsn_ < lsn)
        {
            if (!flush_in_progress_)
            {
                flush_in_progress_ = true;

                lk.unlock();
                try
                {
                    flush();
                }
                catch (...)
                {
                    lk.lock();
                    flush_in_progress_ = false;
                    cv_.notify_all();
                    throw;
                }
                lk.lock();

                flush_in_progress_ = false;
                cv_.notify_all();
            }
            else
            {
                cv_.wait(lk, [&] { return !flush_in_progress_ || flushed_lsn_ >= lsn; });
            }
        }
    }

    void
    FileWalManager::commit_wait(LSN lsn)
    {
        wait_for_durable(lsn);
    }

    void
    FileWalManager::flush()
    {
        DbGuard guard(*db_mutex_);
        std::vector<WALRecord> to_flush;
        {
            std::lock_guard lk(mtx_);
            if (dirty_.empty())
                return;

            to_flush.swap(dirty_);
        }

        write_logs(to_flush);

        auto batch_max_lsn = std::visit([](const auto& rec) { return rec.lsn; }, to_flush.back());

        std::lock_guard lk(mtx_);
        flushed_.insert(flushed_.end(), to_flush.begin(), to_flush.end());
        if (batch_max_lsn > flushed_lsn_)
            flushed_lsn_ = batch_max_lsn;
    }

    void
    FileWalManager::sync()
    {
        DbGuard guard(*db_mutex_);
        flush();
    }

    std::vector<WALRecord>
    FileWalManager::read_all_logs()
    {
        DbGuard guard(*db_mutex_);
        std::lock_guard lk(mtx_);

        // Return all cached records (flushed from disk + dirty in memory)
        std::vector<WALRecord> all_logs;
        all_logs.reserve(flushed_.size() + dirty_.size());

        // Add all flushed records (already on disk, loaded at startup)
        all_logs.insert(all_logs.end(), flushed_.begin(), flushed_.end());

        // Add all dirty records (not yet flushed to disk)
        all_logs.insert(all_logs.end(), dirty_.begin(), dirty_.end());

        return all_logs;
    }

    LSN
    FileWalManager::get_next_lsn() const
    {
        DbGuard guard(*db_mutex_);
        std::lock_guard lk(mtx_);
        return next_lsn_;
    }

    void
    FileWalManager::write_logs(const std::vector<WALRecord>& logs)
    {
        DbGuard guard(*db_mutex_);
        auto dir = storage::path_db_wal(db_path_, db_name_);
        if (!fs::exists(dir))
            fs::create_directories(dir);

        int fd = -1;
        uint64_t opened_first_lsn = 0;

        for (const auto& record : logs)
        {
            auto record_lsn = std::visit([](const auto& rec) { return rec.lsn; }, record);
            uint64_t file_first_lsn =
                ((record_lsn - 1) / MAX_RECORDS_PER_LOGFILE) * MAX_RECORDS_PER_LOGFILE + 1;
            uint64_t file_last_lsn = file_first_lsn + MAX_RECORDS_PER_LOGFILE - 1;

            if (fd < 0 || opened_first_lsn != file_first_lsn)
            {
                if (fd >= 0)
                {
                    fsync(fd);
                    close(fd);
                }

                auto file_path =
                    storage::path_db_wal_logfile(db_path_, db_name_, file_first_lsn, file_last_lsn);

                fd = open(file_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd < 0)
                    throw std::runtime_error("Failed to open WAL log file");

                opened_first_lsn = file_first_lsn;
            }

            auto serialized = serializer_->serialize(record);

            uint64_t record_size = serialized.size();

            if (write(fd, &record_size, sizeof(record_size)) != sizeof(record_size))
                throw std::runtime_error("Failed to write WAL record size");

            if (write(fd, serialized.data(), serialized.size()) != (ssize_t)serialized.size())
                throw std::runtime_error("Failed to write WAL record");
        }

        if (fd >= 0)
        {
            fsync(fd);
            close(fd);
        }
    }

    std::vector<WALRecord>
    FileWalManager::read_logs_from_file(const fs::path& file_path)
    {
        DbGuard guard(*db_mutex_);
        std::vector<WALRecord> logs;

        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open())
            return logs;

        // Прочитать весь файл в буфер
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(file_size);
        file.read(reinterpret_cast<char*>(buffer.data()), file_size);
        file.close();

        // Десериализовать все записи из буфера
        ReadOnlyMemoryStream stream(buffer);

        while (stream.remaining() > 0)
        {
            uint64_t record_size;
            if (!stream.read(&record_size, sizeof(record_size)))
                break;

            if (stream.remaining() < record_size)
                break;

            WALRecord record;
            if (serializer_->deserialize(stream, record))
            {
                logs.push_back(std::move(record));
            }
            else
            {
                throw std::runtime_error(
                    "FileWalManager::read_logs_from_file: failed to deserialize a record"
                );
            }
        }

        return logs;
    }

} // namespace wal
