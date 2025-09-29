#pragma once

#include "../objects/meta_object.hpp"
#include "../objects/data_object.hpp"
#include "wal_object.hpp"
#include "wal_writer.hpp"
#include <chrono>
#include <thread>

namespace storage {
    class wal_manager {
        std::vector<WalRecord> log_;

        std::vector<WalRecord> buffer_;
        std::mutex buffer_mtx_;

        std::thread bg_writer_;
        WalWriter writer_;

        std::chrono::duration<int64_t> checkpoint_interval_ = std::chrono::minutes(1);

        void 
        run_bg_writer();

        void
        insert_to_buffer(const WalRecord& record);

    public:
        void
        push_create_schema(const meta_schema& schema);
        void
        push_drop_schema(const meta_schema& schema);
        void
        push_create_table(const meta_table& table);

        void 
        push_insert(meta_table& table, const data_row& row);

        void 
        write_update_by_filter();

        void 
        write_delete_by_filter();
    };
}