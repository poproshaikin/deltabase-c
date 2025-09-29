#include "include/wal/wal_manager.hpp"
#include "include/wal/wal_object.hpp"
#include <thread>

namespace storage {
    void
    wal_manager::run_bg_writer() {
        auto worker = [this]() {
            while (true) {
                std::this_thread::sleep_for(checkpoint_interval_);

                if (buffer_.size() == 0) {
                    continue;
                }

                buffer_mtx_.lock();
                auto copy = buffer_;
                buffer_.clear();
                buffer_mtx_.unlock();

               writer_.write_batch(copy);               
               log_.insert(log_.end(), copy.begin(), copy.end());
            }
        };

        bg_writer_ = std::thread(worker);
    }

    void
    wal_manager::insert_to_buffer(const WalRecord& record) {
        buffer_mtx_.lock();
        buffer_.push_back(record);
        buffer_mtx_.unlock();
    }

    void
    wal_manager::push_insert(meta_table& table, const data_row& row) {
        insert_record record {
            .table_id = table.id,
            .serialized_row = row.serialize()
        };

        insert_to_buffer(record);
    }

    void
    wal_manager::push_create_schema(const meta_schema& schema) {
        create_schema_record record {
            .schema_id = schema.id,
            .serialized_schema = schema.serialize()
        };

        insert_to_buffer(record);
    }

    void
    wal_manager::push_drop_schema(const meta_schema& schema) {
        drop_schema_record record {
            .schema_id = schema.id
        };

        insert_to_buffer(record);
    }

    void
    wal_manager::push_create_table(const meta_table& table) {
        create_table_record record {
            .table_id = table.id,
            .serialized_table = table.serialize()
        };

        insert_to_buffer(record);
    }
}