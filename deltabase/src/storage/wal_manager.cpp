#include "include/wal/wal_manager.hpp"
#include "include/file_manager.hpp"
#include "include/wal/wal_object.hpp"
#include "shared.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <mutex>
#include <stdexcept>

namespace storage
{
    WalManager::WalManager(std::string db_name, FileManager& fm) : db_name_(db_name), fm_(fm) {
        load_log();
    }

    void
    WalManager::load_log()
    {
        // TODO: Implement proper WAL loading when file_manager provides access to data_dir
        // For now, start with an empty log
        log_.clear();
    }

    wal_logfile*
    WalManager::last_logfile() {
        if (log_.empty()) {
            return nullptr;
        }

        auto* last_lf = &log_[0];
        for (auto& lf : log_) 
            if (lf.last_lsn() > last_lf->last_lsn())
                last_lf = &lf;

        return last_lf;
    }

    wal_logfile
    WalManager::create_logfile() {
        uint64_t lsn = 0;

        auto* last_lf = last_logfile();
        if (last_lf) {
            lsn = last_lf->last_lsn();
        }

        auto file = fm_.create_wal_logfile(db_name_, lsn, lsn);
        file.first.close();

        return wal_logfile(file.second);
    }

    void
    WalManager::flush_on_disk() {
        if (buffer_.empty())
            return;

        // безопасно забираем буфер
        std::vector<wal_record> buf_copy;
        {
            std::scoped_lock lock(buffer_mtx_);
            buf_copy = std::move(buffer_);
            buffer_.clear();
        }

        // создаём временный логфайл для записи
        wal_logfile* logfile_to_write = last_logfile();
        std::vector<std::pair<fs::path, bytes_v>> files_to_write;
        
        // если нет существующих логфайлов, создаём новый
        if (!logfile_to_write) {
            auto file = fm_.create_wal_logfile(db_name, 1, 1);
            file.first.close();
            
            // создаём временный логфайл для работы
            static thread_local wal_logfile temp_logfile(file.second);
            temp_logfile.records.clear();
            logfile_to_write = &temp_logfile;
        }

        // получаем текущий LSN для присвоения записям
        uint64_t current_lsn = logfile_to_write->records.empty() ? 0 : logfile_to_write->last_lsn();

        for (auto& record : buf_copy) {
            // присваиваем LSN записи
            ++current_lsn;
            std::visit([current_lsn](auto& r) { r.lsn = current_lsn; }, record);

            uint64_t record_size = std::visit([](const auto& r) { return r.estimate_size(); }, record);

            // проверяем, помещается ли запись в текущий логфайл
            if (logfile_to_write->size() + record_size > wal_logfile::max_logfile_size) {
                // сохраняем текущий файл для записи
                if (!logfile_to_write->records.empty()) {
                    files_to_write.emplace_back(logfile_to_write->path, logfile_to_write->serialize());
                }
                
                // создаём новый логфайл
                auto file = fm_.create_wal_logfile(db_name, current_lsn, current_lsn);
                file.first.close();
                
                // создаём новый временный логфайл для работы
                static thread_local wal_logfile new_temp_logfile(file.second);
                new_temp_logfile.records.clear();
                logfile_to_write = &new_temp_logfile;
            }

            // добавляем запись в логфайл
            logfile_to_write->records.push_back(std::move(record));
        }
        
        // добавляем последний файл для записи
        if (!logfile_to_write->records.empty()) {
            files_to_write.emplace_back(logfile_to_write->path, logfile_to_write->serialize());
        }

        // записываем все файлы на диск
        for (const auto& [path, data] : files_to_write) {
            try {
                std::ofstream stream(path, std::ios::binary | std::ios::trunc);
                if (!stream.is_open()) {
                    throw std::runtime_error("Failed to open WAL file: " + path.string());
                }

                stream.write(reinterpret_cast<const char*>(data.data()), data.size());
                stream.flush();
                
                if (stream.fail()) {
                    throw std::runtime_error("Failed to write WAL file: " + path.string());
                }
            } catch (const std::exception& e) {
                std::cerr << "Error writing WAL file " << path << ": " << e.what() << std::endl;
                throw;
            }
        }
    }

    void
    WalManager::insert_to_buffer(const wal_record& record) {
        buffer_mtx_.lock();
        buffer_.push_back(record);
        buffer_mtx_.unlock();
    }

    void
    WalManager::push_insert(MetaTable& table, const DataRow& row) {
        insert_record record {
            .table_id = table.id,
            .serialized_row = row.serialize()
        };

        insert_to_buffer(record);
    }

    void
    WalManager::push_create_schema(const MetaSchema& schema) {
        create_schema_record record {
            .schema_id = schema.id,
            .serialized_schema = schema.serialize()
        };

        insert_to_buffer(record);
    }

    void
    WalManager::push_drop_schema(const MetaSchema& schema) {
        drop_schema_record record {
            .schema_id = schema.id
        };

        insert_to_buffer(record);
    }

    void
    WalManager::push_create_table(const MetaTable& table) {
        create_table_record record {
            .table_id = table.id,
            .serialized_table = table.serialize()
        };

        insert_to_buffer(record);
    }
} // namespace storage