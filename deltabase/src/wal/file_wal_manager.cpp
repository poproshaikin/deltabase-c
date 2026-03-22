//
// Created by poproshaikin on 06.03.26.
//

#include "include/file_wal_manager.hpp"

#include "include/wal_serializer_factory.hpp"
#include "path.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <fcntl.h>

namespace wal
{
    using namespace types;
    using namespace misc;

    FileWalManager::FileWalManager(
        const fs::path& db_path, const std::string& db_name, Config::SerializerType serializer_type
    )
        : db_path_(db_path), db_name_(db_name), next_lsn_(1)
    {
        WalSerializerFactory factory;
        serializer_ = factory.make(serializer_type);

        // Create WAL directory if it doesn't exist
        auto wal_dir = storage::path_db_wal(db_path_, db_name_);
        if (!fs::exists(wal_dir))
        {
            fs::create_directories(wal_dir);
        }

        // Ensure a first WAL file exists for an empty WAL directory.
        if (fs::is_empty(wal_dir))
        {
            auto first_file_path = storage::path_db_wal_logfile(
                db_path_, db_name_, 1, MAX_RECORDS_PER_LOGFILE
            );

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
            [](const WalRecord& lhs, const WalRecord& rhs)
            {
                auto l_lsn = std::visit([](const auto& rec) { return rec.lsn; }, lhs);
                auto r_lsn = std::visit([](const auto& rec) { return rec.lsn; }, rhs);
                return l_lsn < r_lsn;
            }
        );
    }

    void
    FileWalManager::append_log(const WalRecord& record)
    {
        auto lsn = next_lsn_++;

        auto record_with_lsn = std::visit(
            [lsn](auto rec) -> WalRecord
            {
                rec.lsn = lsn;
                return rec;
            },
            record
        );

        dirty_.push_back(record_with_lsn);
    }

    void
    FileWalManager::append_log(const std::vector<WalRecord>& records)
    {
        for (const auto& record : records)
        {
            append_log(record);
        }
    }

    WalRecord
    FileWalManager::read_log(Lsn lsn)
    {
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
    FileWalManager::flush()
    {
        if (dirty_.empty())
            return;

        write_logs(dirty_);
        flushed_.insert(flushed_.end(), dirty_.begin(), dirty_.end());
        dirty_.clear();
    }

    void
    FileWalManager::sync()
    {
        flush();
    }

    std::vector<WalRecord>
    FileWalManager::read_all_logs()
    {
        // Return all cached records (flushed from disk + dirty in memory)
        std::vector<WalRecord> all_logs;
        all_logs.reserve(flushed_.size() + dirty_.size());

        // Add all flushed records (already on disk, loaded at startup)
        all_logs.insert(all_logs.end(), flushed_.begin(), flushed_.end());

        // Add all dirty records (not yet flushed to disk)
        all_logs.insert(all_logs.end(), dirty_.begin(), dirty_.end());

        return all_logs;
    }

    Lsn
    FileWalManager::get_next_lsn() const
    {
        return next_lsn_;
    }

    void
    FileWalManager::write_logs(const std::vector<WalRecord>& logs)
    {
        auto dir = storage::path_db_wal(db_path_, db_name_);
        if (!fs::exists(dir))
            fs::create_directories(dir);

        int fd = -1;
        uint64_t opened_first_lsn = 0;

        for (const auto& record : logs)
        {
            auto record_lsn = std::visit([](const auto& rec) { return rec.lsn; }, record);
            uint64_t file_first_lsn = ((record_lsn - 1) / MAX_RECORDS_PER_LOGFILE)
                * MAX_RECORDS_PER_LOGFILE + 1;
            uint64_t file_last_lsn = file_first_lsn + MAX_RECORDS_PER_LOGFILE - 1;

            if (fd < 0 || opened_first_lsn != file_first_lsn)
            {
                if (fd >= 0)
                {
                    fsync(fd);
                    close(fd);
                }

                auto file_path = storage::path_db_wal_logfile(
                    db_path_, db_name_, file_first_lsn, file_last_lsn
                );

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

    std::vector<WalRecord>
    FileWalManager::read_logs_from_file(const fs::path& file_path)
    {
        std::vector<WalRecord> logs;

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
            // Прочитать размер записи
            uint64_t record_size;
            if (!stream.read(&record_size, sizeof(record_size)))
                break;

            // Проверить что хватает данных
            if (stream.remaining() < record_size)
                break;

            // Десериализовать запись
            WalRecord record;
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
