#include "include/wal/wal_manager.hpp"
#include "include/wal/wal_record.hpp"
#include <thread>

namespace storage {
    void
    WalManager::run_bg_writer() {
        auto worker = [this]() {
            while (true) {
                std::this_thread::sleep_for(flush_interval_);

                if (buffer_.size() == 0) {
                    continue;
                }

                buffer_mtx_.lock();
                auto copy = buffer_;
                buffer_.clear();
                buffer_mtx_.unlock();

                writer_.write_batch(copy);               
            }
        };

        bg_writer_ = std::thread(worker);
    }

    void
    WalManager::insert_to_buffer(const WalRecord& record) {
        buffer_mtx_.lock();
        buffer_.push_back(record);
        buffer_mtx_.unlock();
    }

    void
    WalManager::push_insert(MetaTable& table, const DataRow& row) {
        InsertRecord record {
            .table_id = table.id,
            .serialized_row = row.serialize()
        };

        insert_to_buffer(record);
    }
}